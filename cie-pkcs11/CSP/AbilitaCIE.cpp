//
//  AbilitaCIE.cpp
//  cie-pkcs11
//
//  Created by ugo chirico on 06/10/18. http://www.ugochirico.com
//  Copyright © 2018 IPZS. All rights reserved.
//

#include "IAS.h"
#include "../PKCS11/wintypes.h"
#include "../PKCS11/PKCS11Functions.h"
#include "../PKCS11/Slot.h"
#include "../Util/ModuleInfo.h"
#include "../Crypto/sha256.h"
#include "../Crypto/sha512.h"
#include <functional>
#include "../Crypto/ASNParser.h"
#include "../PCSC/PCSC.h"
#include <string>
#include "AbilitaCIE.h"
#include <string>
#include "../Cryptopp/misc.h"

#include "../Crypto/ASNParser.h"
#include <stdio.h>
#include "../Crypto/AES.h"
#include "../Cryptopp/cryptlib.h"
#include "../Cryptopp/asn.h"
#include "../Util/CryptoppUtils.h"
#include "../Sign/CIESign.h"
#include "../Sign/CIEVerify.h"

#define ROLE_USER                   1
#define ROLE_ADMIN                  2
#define CARD_ALREADY_ENABLED        0x000000F0
#define CARD_PAN_MISMATCH            (int)(0x000000F1)

OID OID_SURNAME = ((OID(2) += 5) += 4) += 4;

OID OID_GIVENNAME = ((OID(2) += 5) += 4) += 42;

extern CModuleInfo moduleInfo;

void GetCertInfo(CryptoPP::BufferedTransformation & certin,
                 std::string & serial,
                 CryptoPP::BufferedTransformation & issuer,
                 CryptoPP::BufferedTransformation & subject,
                 std::string & notBefore,
                 std::string & notAfter,
                 CryptoPP::Integer& mod,
                 CryptoPP::Integer& pubExp);

std::vector<word32> fromObjectIdentifier(std::string sObjId);

int TokenTransmitCallback(safeConnection *data, uint8_t *apdu, DWORD apduSize, uint8_t *resp, DWORD *respSize);

DWORD CardAuthenticateEx(IAS*       ias,
                        DWORD       PinId,
                        DWORD       dwFlags,
                        BYTE*       pbPinData,
                        DWORD       cbPinData,
                        BYTE*       *ppbSessionPin,
                        DWORD*      pcbSessionPin,
                        int*        pcAttemptsRemaining);

extern "C" {
    CK_RV CK_ENTRY AbilitaCIE(const char*  szPAN, const char*  szPIN, int* attempts, PROGRESS_CALLBACK progressCallBack, COMPLETED_CALLBACK completedCallBack);
    CK_RV CK_ENTRY VerificaCIEAbilitata(const char*  szPAN);
    CK_RV CK_ENTRY DisabilitaCIE(const char*  szPAN);
    CK_RV CK_ENTRY verificaConCIE(const char* inFilePath);
    CK_RV CK_ENTRY firmaConCIE(const char* inFilePath, const char* type, const char* pin, const char* pan, int page, float x, float y, float w, float h, const char* imagePathFile, const char* outFilePath, PROGRESS_CALLBACK progressCallBack, SIGN_COMPLETED_CALLBACK completedCallBack);
    CK_RV CK_ENTRY getVerifyInfo(int index, struct verifyInfo_t* vInfos);
    CK_RV CK_ENTRY getNumberOfSign();
}


VERIFY_RESULT verifyResult;

CK_RV CK_ENTRY  getNumberOfSign()
{
    return (CK_RV)verifyResult.verifyInfo.pSignerInfos->nCount;
}

CK_RV CK_ENTRY getVerifyInfo(int index, struct verifyInfo_t* vInfos)
{
    if(index < verifyResult.verifyInfo.pSignerInfos->nCount)
    {
        SIGNER_INFO tmpSignerInfo = (verifyResult.verifyInfo.pSignerInfos->pSignerInfo)[index];// +(index * sizeof(SIGNER_INFO)));
        strcpy(vInfos->name, tmpSignerInfo.szGIVENNAME);
        strcpy(vInfos->surname, tmpSignerInfo.szSURNAME);
        strcpy(vInfos->cn, tmpSignerInfo.szCN);
        strcpy(vInfos->cadn, tmpSignerInfo.szCADN);
        strcpy(vInfos->signingTime, tmpSignerInfo.szSigningTime);
        vInfos->CertRevocStatus = tmpSignerInfo.pRevocationInfo->nRevocationStatus;
        vInfos->isCertValid = (tmpSignerInfo.bitmask & VERIFIED_CERT_GOOD) == VERIFIED_CERT_GOOD;
        vInfos->isSignValid = (tmpSignerInfo.bitmask & VERIFIED_SIGNATURE) == VERIFIED_SIGNATURE;
    }
    
    return 0;
}

CK_RV CK_ENTRY verificaConCIE(const char* inFilePath)
{
    CIEVerify* verifier = new CIEVerify();

    verifier->verify(inFilePath, (VERIFY_RESULT*)&verifyResult);

    if (verifyResult.nErrorCode == 0)
    {
        return 0;
    }
    else
    {
        printf("Errore nella verifica: %lu\n", verifyResult.nErrorCode);
        return verifyResult.nErrorCode;
    }

}

CK_RV CK_ENTRY firmaConCIE(const char* inFilePath, const char* type, const char* pin, const char* pan, int page, float x, float y, float w, float h, const char* imagePathFile, const char* outFilePath, PROGRESS_CALLBACK progressCallBack, SIGN_COMPLETED_CALLBACK completedCallBack)
{

    printf("page: %d, x: %f, y: %f, w: %f, h: %f", page, x, y, w, h);

    char* readers = NULL;
    char* ATR = NULL;
    try
    {
        std::map<uint8_t, ByteDynArray> hashSet;

        DWORD len = 0;
        ByteDynArray CertCIE;
        ByteDynArray SOD;

        SCARDCONTEXT hSC;

        long nRet = SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &hSC);
        if (nRet != SCARD_S_SUCCESS)
            return CKR_DEVICE_ERROR;

        OutputDebugString("Establish Context ok\n");

        if (SCardListReaders(hSC, nullptr, NULL, &len) != SCARD_S_SUCCESS) {
            OutputDebugString("List readers ko\n");
            return CKR_TOKEN_NOT_PRESENT;
        }

        if (len == 1)
            return CKR_TOKEN_NOT_PRESENT;

        readers = (char*)malloc(len);

        if (SCardListReaders(hSC, nullptr, (char*)readers, &len) != SCARD_S_SUCCESS) {
            free(readers);
            return CKR_TOKEN_NOT_PRESENT;
        }

        char *curreader = readers;
        bool foundCIE = false;
        for (; curreader[0] != 0; curreader += strnlen(curreader, len) + 1)
        {
            safeConnection conn(hSC, curreader, SCARD_SHARE_SHARED);
            if (!conn.hCard)
                continue;

            uint32_t atrLen = 40;
            if(SCardGetAttrib(conn.hCard, SCARD_ATTR_ATR_STRING, (uint8_t*)ATR, &atrLen) != SCARD_S_SUCCESS) {
                free(readers);
                return CKR_DEVICE_ERROR;
            }
            
            ATR = (char*)malloc(atrLen);
            
            if(SCardGetAttrib(conn.hCard, SCARD_ATTR_ATR_STRING, (uint8_t*)ATR, &atrLen) != SCARD_S_SUCCESS) {
                free(readers);
                free(ATR);
                return CKR_DEVICE_ERROR;
            }
            
            ByteArray atrBa((BYTE*)ATR, atrLen);

            progressCallBack(20, "");

            IAS* ias = new IAS((CToken::TokenTransmitCallback)TokenTransmitCallback, atrBa);
            ias->SetCardContext(&conn);
            
            foundCIE = false;
            ias->token.Reset();
            ias->SelectAID_IAS();
            ias->ReadPAN();
            
            foundCIE = true;
            ByteDynArray IntAuth;
            ias->SelectAID_CIE();
            ias->ReadDappPubKey(IntAuth);
            ias->SelectAID_CIE();
            ias->InitEncKey();

            ByteDynArray IdServizi;
            ias->ReadIdServizi(IdServizi);
            ByteArray baPan = ByteArray((uint8_t*)pan, strlen(pan));

            if (memcmp(baPan.data(), IdServizi.data(), IdServizi.size()) != 0)
            {
                return CARD_PAN_MISMATCH;
            }
            
            ByteDynArray FullPIN;
            ByteArray LastPIN = ByteArray((uint8_t*)pin, strlen(pin));
            ias->GetFirstPIN(FullPIN);
            FullPIN.append(LastPIN);
            ias->token.Reset();
            
            progressCallBack(40, "");

            char fullPinCStr[9];
            memcpy(fullPinCStr, FullPIN.data(), 8);
            fullPinCStr[8] = 0;

            CIESign* cieSign = new CIESign(ias);

            uint16_t ret = cieSign->sign(inFilePath, type, fullPinCStr, page, x, y, w, h, imagePathFile, outFilePath);
            if((ret & (0x63C0)) == 0x63C0)
            {
                return CKR_PIN_INCORRECT;
            }else if (ret == 0x6983)
            {
                return CKR_PIN_LOCKED;
            }
            
            
            progressCallBack(100, "");
            
            OutputDebugString("CieSign ret: %d", ret);

            free(ias);
            free(cieSign);

            completedCallBack(ret);
        }
        
        if (!foundCIE) {
            free(ATR);
            free(readers);
            return CKR_TOKEN_NOT_RECOGNIZED;
            
        }
    }
    catch (std::exception &ex) {
        OutputDebugString(ex.what());
        if (ATR)
            free(ATR);
        OutputDebugString("Eccezione: %s", ex.what());
        if (readers)
            free(readers);

        OutputDebugString("General error\n");
        return CKR_GENERAL_ERROR;
    }

    if (ATR)
        free(ATR);

    free(readers);
    return SCARD_S_SUCCESS;
}



CK_RV CK_ENTRY VerificaCIEAbilitata(const char*  szPAN)
{
    try {
        if(IAS::IsEnrolled(szPAN))
            return 1;
        else
            return 0;
    }
    catch(...)
    {
        return CKR_GENERAL_ERROR;
    }
    
    return CKR_TOKEN_NOT_PRESENT;
    
}

CK_RV CK_ENTRY DisabilitaCIE(const char*  szPAN)
{
    try
    {
        if(IAS::IsEnrolled(szPAN))
        {
            IAS::Unenroll(szPAN);
            return CKR_OK;
        }
        else
        {
            return CKR_FUNCTION_FAILED;
        }
    }
    catch(...)
    {
        return CKR_GENERAL_ERROR;
    }
    return CKR_TOKEN_NOT_PRESENT;
}

CK_RV CK_ENTRY AbilitaCIE(const char*  szPAN, const char*  szPIN, int* attempts, PROGRESS_CALLBACK progressCallBack, COMPLETED_CALLBACK completedCallBack)
{
    char* readers = NULL;
    char* ATR = NULL;
	try
    {
		std::map<uint8_t, ByteDynArray> hashSet;
		
		DWORD len = 0;
		ByteDynArray CertCIE;
		ByteDynArray SOD;
		ByteDynArray IdServizi;
		
		SCARDCONTEXT hSC;

        progressCallBack(1, "Connessione alla CIE");
        
		long nRet = SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &hSC);
        if(nRet != SCARD_S_SUCCESS)
            return CKR_DEVICE_ERROR;
        
        if (SCardListReaders(hSC, nullptr, NULL, &len) != SCARD_S_SUCCESS) {
            return CKR_TOKEN_NOT_PRESENT;
        }
        
        if(len == 1)
            return CKR_TOKEN_NOT_PRESENT;
        
        readers = (char*)malloc(len);
        
        if (SCardListReaders(hSC, nullptr, (char*)readers, &len) != SCARD_S_SUCCESS) {
            free(readers);
            return CKR_TOKEN_NOT_PRESENT;
        }

        progressCallBack(5, "CIE Connessa");
        
		char *curreader = readers;
		bool foundCIE = false;
		for (; curreader[0] != 0; curreader += strnlen(curreader, len) + 1)
        {
            safeConnection conn(hSC, curreader, SCARD_SHARE_SHARED);
            if (!conn.hCard)
                continue;

            uint32_t atrLen = 40;
            if(SCardGetAttrib(conn.hCard, SCARD_ATTR_ATR_STRING, (uint8_t*)ATR, &atrLen) != SCARD_S_SUCCESS) {
                free(readers);
                return CKR_DEVICE_ERROR;
            }
            
            ATR = (char*)malloc(atrLen);
            
            if(SCardGetAttrib(conn.hCard, SCARD_ATTR_ATR_STRING, (uint8_t*)ATR, &atrLen) != SCARD_S_SUCCESS) {
                free(readers);
                free(ATR);
                return CKR_DEVICE_ERROR;
            }
            
            ByteArray atrBa((BYTE*)ATR, atrLen);
            
            progressCallBack(10, "Verifica carta esistente");
            
            IAS ias((CToken::TokenTransmitCallback)TokenTransmitCallback, atrBa);
            ias.SetCardContext(&conn);
            
            foundCIE = false;
            
            ias.token.Reset();
            ias.SelectAID_IAS();
            ias.ReadPAN();
                    
            ByteDynArray IntAuth;
            ias.SelectAID_CIE();
            ias.ReadDappPubKey(IntAuth);
            //ias.SelectAID_CIE();
            ias.InitEncKey();
            
            ByteDynArray IdServizi;
            ias.ReadIdServizi(IdServizi);
            
            if (ias.IsEnrolled())
            {
                return CARD_ALREADY_ENABLED;
            }

            progressCallBack(15, "Lettura dati dalla CIE");

            ByteArray serviziData(IdServizi.left(12));

            ByteDynArray SOD;
            ias.ReadSOD(SOD);
            uint8_t digest = ias.GetSODDigestAlg(SOD);
                        
            ByteArray intAuthData(IntAuth.left(GetASN1DataLenght(IntAuth)));
            
			ByteDynArray IntAuthServizi;
            ias.ReadServiziPubKey(IntAuthServizi);
            ByteArray intAuthServiziData(IntAuthServizi.left(GetASN1DataLenght(IntAuthServizi)));

            ias.SelectAID_IAS();
            ByteDynArray DH;
            ias.ReadDH(DH);
            ByteArray dhData(DH.left(GetASN1DataLenght(DH)));

            if (szPAN && IdServizi != ByteArray((uint8_t*)szPAN, strnlen(szPAN, 20)))
                continue;

            foundCIE = true;
            
            progressCallBack(20, "Autenticazione...");
            
            DWORD rs = CardAuthenticateEx(&ias, ROLE_USER, FULL_PIN, (BYTE*)szPIN, (DWORD)strnlen(szPIN, sizeof(szPIN)), nullptr, 0, attempts);
            if (rs == SCARD_W_WRONG_CHV)
            {
                free(ATR);
                free(readers);
                return CKR_PIN_INCORRECT;
            }
            else if (rs == SCARD_W_CHV_BLOCKED)
            {
                free(ATR);
                free(readers);
                return CKR_PIN_LOCKED;
            }
            else if (rs != SCARD_S_SUCCESS)
            {
                free(ATR);
                free(readers);
                return CKR_GENERAL_ERROR;
            }
            
            
            progressCallBack(45, "Lettura seriale");
            
            ByteDynArray Serial;
            ias.ReadSerialeCIE(Serial);
            ByteArray serialData = Serial.left(9);
            
            std::string st_serial((char*)serialData.data(), serialData.size());
            
            progressCallBack(55, "Lettura certificato");
            
            ByteDynArray CertCIE;
            ias.ReadCertCIE(CertCIE);
            ByteArray certCIEData = CertCIE.left(GetASN1DataLenght(CertCIE));
            
            if (digest == 1)
            {
                CSHA256 sha256;
                hashSet[0xa1] = sha256.Digest(serviziData);
                hashSet[0xa4] = sha256.Digest(intAuthData);
                hashSet[0xa5] = sha256.Digest(intAuthServiziData);
                hashSet[0x1b] = sha256.Digest(dhData);
                hashSet[0xa2] = sha256.Digest(serialData);
                hashSet[0xa3] = sha256.Digest(certCIEData);
                ias.VerificaSOD(SOD, hashSet);

            }
            else
            {
                CSHA512 sha512;
                hashSet[0xa1] = sha512.Digest(serviziData);
                hashSet[0xa4] = sha512.Digest(intAuthData);
                hashSet[0xa5] = sha512.Digest(intAuthServiziData);
                hashSet[0x1b] = sha512.Digest(dhData);
                hashSet[0xa2] = sha512.Digest(serialData);
                hashSet[0xa3] = sha512.Digest(certCIEData);
                ias.VerificaSODPSS(SOD, hashSet);
            }
            
            ByteArray pinBa((uint8_t*)szPIN, 4);
            
            progressCallBack(85, "Memorizzazione in cache");
            
            std::string sidServizi((char*)IdServizi.data(), IdServizi.size());
            
            ias.SetCache(sidServizi.c_str(), CertCIE, pinBa);
            
            std::string span(sidServizi.c_str());
            std::string name;
            std::string surname;
            
            CryptoPP::ByteQueue certin;
            certin.Put(CertCIE.data(),CertCIE.size());
            
            std::string serial;
            CryptoPP::ByteQueue issuer;
            CryptoPP::ByteQueue subject;
            std::string notBefore;
            std::string notAfter;
            CryptoPP::Integer mod;
            CryptoPP::Integer pubExp;
            
            GetCertInfo(certin, serial, issuer, subject, notBefore, notAfter, mod, pubExp);
            
            CryptoPP::BERSequenceDecoder subjectEncoder(subject);
            {
                while(!subjectEncoder.EndReached())
                {
                    CryptoPP::BERSetDecoder item(subjectEncoder);
                    CryptoPP::BERSequenceDecoder attributes(item); {
                        
                        OID oid(attributes);
                        if(oid == OID_GIVENNAME)
                        {
                            byte tag = 0;
                            attributes.Peek(tag);
                            
                            CryptoPP::BERDecodeTextString(
                                                          attributes,
                                                          name,
                                                          tag);
                        }
                        else if(oid == OID_SURNAME)
                        {
                            byte tag = 0;
                            attributes.Peek(tag);
                            
                            CryptoPP::BERDecodeTextString(
                                                          attributes,
                                                          surname,
                                                          tag);
                        }
                        
                        item.SkipAll();
                    }
                }
            }
        
            subjectEncoder.SkipAll();
            
            std::string fullname = name + " " + surname;
            completedCallBack(span, fullname, st_serial);
            break;
		}
        
		if (!foundCIE) {
            free(ATR);
            free(readers);
            return CKR_TOKEN_NOT_RECOGNIZED;
            
		}

	}
	catch (std::exception &ex) {
		OutputDebugString(ex.what());
        if(ATR)
            free(ATR);
        
        if(readers)
            free(readers);
        return CKR_GENERAL_ERROR;
	}

    if(ATR)
        free(ATR);
    
    free(readers);
    
    progressCallBack(100, "");
    
    return SCARD_S_SUCCESS;
}



DWORD CardAuthenticateEx(IAS*       ias,
                         DWORD       PinId,
                         DWORD       dwFlags,
                         BYTE*       pbPinData,
                         DWORD       cbPinData,
                         BYTE*       *ppbSessionPin,
                         DWORD*      pcbSessionPin,
                         int*      pcAttemptsRemaining) {
    
    ias->SelectAID_IAS();
    ias->SelectAID_CIE();
    
    
    // leggo i parametri di dominio DH e della chiave di extauth
    ias->InitDHParam();
    
    ByteDynArray dappData;
    ias->ReadDappPubKey(dappData);
    
    ias->InitExtAuthKeyParam();
    
    
    ias->DHKeyExchange();
    
    // DAPP
    ias->DAPP();
    
    // verifica PIN
    StatusWord sw;
    if (PinId == ROLE_USER) {
        
        ByteDynArray PIN;
        if ((dwFlags & FULL_PIN) != FULL_PIN)
            ias->GetFirstPIN(PIN);
        PIN.append(ByteArray(pbPinData, cbPinData));
        sw = ias->VerifyPIN(PIN);
    }
    else if (PinId == ROLE_ADMIN) {
        ByteArray pinBa(pbPinData, cbPinData);
        sw = ias->VerifyPUK(pinBa);
    }
    else
        return SCARD_E_INVALID_PARAMETER;
    
    if (sw == 0x6983) {
        if (PinId == ROLE_USER)
            ias->IconaSbloccoPIN();
        return SCARD_W_CHV_BLOCKED;
    }
    if (sw >= 0x63C0 && sw <= 0x63CF) {
        if (pcAttemptsRemaining!=nullptr)
            *pcAttemptsRemaining = sw - 0x63C0;
        return SCARD_W_WRONG_CHV;
    }
    if (sw == 0x6700) {
        return SCARD_W_WRONG_CHV;
    }
    if (sw == 0x6300)
        return SCARD_W_WRONG_CHV;
    if (sw != 0x9000) {
        throw scard_error(sw);
    }
    
    return SCARD_S_SUCCESS;
}

int TokenTransmitCallback(safeConnection *conn, BYTE *apdu, DWORD apduSize, BYTE *resp, DWORD *respSize) {
    if (apduSize == 2) {
        WORD code = *(WORD*)apdu;
        if (code == 0xfffd) {
            long bufLen = *respSize;
            *respSize = sizeof(conn->hCard)+2;
            CryptoPP::memcpy_s(resp, bufLen, &conn->hCard, sizeof(conn->hCard));
            resp[sizeof(&conn->hCard)] = 0;
            resp[sizeof(&conn->hCard) + 1] = 0;
            
            return SCARD_S_SUCCESS;
        }
        else if (code == 0xfffe) {
            DWORD protocol = 0;
            ODS("UNPOWER CARD");
            auto ris = SCardReconnect(conn->hCard, SCARD_SHARE_SHARED, SCARD_PROTOCOL_Tx, SCARD_UNPOWER_CARD, &protocol);
            
            
            if (ris == SCARD_S_SUCCESS) {
                SCardBeginTransaction(conn->hCard);
                *respSize = 2;
                resp[0] = 0x90;
                resp[1] = 0x00;
            }
            return ris;
        }
        else if (code == 0xffff) {
            DWORD protocol = 0;
            auto ris = SCardReconnect(conn->hCard, SCARD_SHARE_SHARED, SCARD_PROTOCOL_Tx, SCARD_RESET_CARD, &protocol);
            if (ris == SCARD_S_SUCCESS) {
                SCardBeginTransaction(conn->hCard);
                *respSize = 2;
                resp[0] = 0x90;
                resp[1] = 0x00;
            }
            ODS("RESET CARD");
            return ris;
        }
    }
    //ODS(String().printf("APDU: %s\n", dumpHexData(ByteArray(apdu, apduSize), String()).lock()).lock());
    auto ris = SCardTransmit(conn->hCard, SCARD_PCI_T1, apdu, apduSize, NULL, resp, respSize);
    if(ris == SCARD_W_RESET_CARD || ris == SCARD_W_UNPOWERED_CARD)
    {
        ODS("card resetted");
        DWORD protocol = 0;
        ris = SCardReconnect(conn->hCard, SCARD_SHARE_SHARED, SCARD_PROTOCOL_Tx, SCARD_LEAVE_CARD, &protocol);
        if (ris != SCARD_S_SUCCESS)
            ODS("Errore reconnect");
        else
            ris = SCardTransmit(conn->hCard, SCARD_PCI_T1, apdu, apduSize, NULL, resp, respSize);
    }
    
    if (ris != SCARD_S_SUCCESS) {
        ODS("Errore trasmissione APDU");
    }
    
    //else
    //ODS(String().printf("RESP: %s\n", dumpHexData(ByteArray(resp, *respSize), String()).lock()).lock());
    
    return ris;
}



std::vector<word32> fromObjectIdentifier(std::string sObjId)
{
    std::vector<word32> out;
    
    int nVal;
    int nAux;
    char* szTok;
    char* szOID = new char[sObjId.size()];
    strcpy(szOID, sObjId.c_str());
    
    szTok = strtok(szOID, ".");
    
    UINT nFirst = 40 * atoi(szTok) + atoi(strtok(NULL, "."));
    if(nFirst > 0xff)
    {
        delete[] szOID;
        throw -1;//new CASN1BadObjectIdException(strObjId);
    }
    
    out.push_back(nFirst);
    
    int i = 0;
    
    while ((szTok = strtok(NULL, ".")) != NULL)
    {
        nVal = atoi(szTok);
        if(nVal == 0)
        {
            out.push_back(0x00);
        }
        else if (nVal == 1)
        {
            out.push_back(0x01);
        }
        else
        {
            i = (int)ceil((log((double)abs(nVal)) / log((double)2)) / 7); // base 128
            while (nVal != 0)
            {
                nAux = (int)(floor(nVal / pow(128, i - 1)));
                nVal = nVal - (int)(pow(128, i - 1) * nAux);
                
                // next value (or with 0x80)
                if(nVal != 0)
                    nAux |= 0x80;

                out.push_back(nAux);
                
                i--;
            }
        }
    }
    
    
    delete[] szOID;

    return out;
}

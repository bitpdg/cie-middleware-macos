//
//  AbilitaCIE.h
//  cie-pkcs11
//
//  Created by ugo chirico on 02/09/18. http://www.ugochirico.com
//  Copyright © 2018 IPZS. All rights reserved.
//
#include "../PKCS11/cryptoki.h"
#include <string>

#define SCARD_ATTR_VALUE(Class, Tag) ((((uint32_t)(Class)) << 16) | ((uint32_t)(Tag)))
#define SCARD_CLASS_ICC_STATE       9   /**< ICC State specific definitions */
#define SCARD_ATTR_ATR_STRING SCARD_ATTR_VALUE(SCARD_CLASS_ICC_STATE, 0x0303) /**< Answer to reset (ATR) string. */

//using namespace std

/* CK_NOTIFY is an application callback that processes events */
typedef CK_CALLBACK_FUNCTION(CK_RV, PROGRESS_CALLBACK)(
                                               const int progress,
                                               const char* szMessage);

typedef CK_CALLBACK_FUNCTION(CK_RV, SIGN_COMPLETED_CALLBACK)(
                                               const int ret);

typedef CK_CALLBACK_FUNCTION(CK_RV, COMPLETED_CALLBACK)(
                                                        std::string& pan,
                                                        std::string& name,
                                                        std::string& ef_cie);

typedef CK_RV (*AbilitaCIEfn)(const char*  szPAN,
                              const char*  szPIN,
                              int* attempts,
                              PROGRESS_CALLBACK progressCallBack,
                              COMPLETED_CALLBACK completedCallBack);

typedef CK_RV (*VerificaCIEAbilitatafn)(const char*  szPAN);
typedef CK_RV (*DisabilitaCIEfn)(const char*  szPAN);
typedef CK_RV (*firmaConCIEfn)(const char* inFilePath, const char* type, const char* pin, const char* pan, int page, float x, float y, float w, float h, const char* imagePathFile, const char* outFilePath, PROGRESS_CALLBACK progressCallBack, SIGN_COMPLETED_CALLBACK completedCallBack);
typedef CK_RV (*verificaConCIEfn)(const char* inFilePath);
typedef CK_RV (*getVerifyInfofn)(int index, struct verifyInfo_t* vInfos);
typedef CK_RV (*getNumberOfSignfn)();


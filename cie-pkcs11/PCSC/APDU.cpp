
#include "apdu.h"
#include "../Util/tlv.h"
#include "../Util/util.h"
#include "../Crypto/des3.h"
#include "../Crypto/mac.h"
#include "Token.h"


static char *szCompiledFile=__FILE__;

APDU::APDU()  {
}
APDU::APDU(BYTE CLA,BYTE INS,BYTE P1,BYTE P2,BYTE LC,BYTE *pData,BYTE LE)  {
	init_func
	if (LC>250) throw;
	btINS=INS;btCLA=CLA;btP1=P1;btP2=P2;btLC=LC;pbtData=pData;btLE=LE;
	bLC=true;bLE=true;
	exit_func
}
APDU::APDU(BYTE CLA,BYTE INS,BYTE P1,BYTE P2,BYTE LC,BYTE *pData)   {
	if (LC>251) throw;
	btINS=INS;btCLA=CLA;btP1=P1;btP2=P2;btLC=LC;pbtData=pData;btLE=0;
	bLC=true;bLE=false;
}
APDU::APDU(BYTE CLA,BYTE INS,BYTE P1,BYTE P2,BYTE LE)   {
	btINS=INS;btCLA=CLA;btP1=P1;btP2=P2;btLE=LE;btLC=0;
	bLC=false;bLE=true;
}
APDU::APDU(BYTE CLA,BYTE INS,BYTE P1,BYTE P2)   {
	btINS=INS;btCLA=CLA;btP1=P1;btP2=P2;btLE=0;btLC=0;
	bLC=false;bLE=false;
}

APDU::~APDU()
{
}

#pragma once

#include "disigonsdk.h"
class CIEVerify
{
	public:
		CIEVerify();
		~CIEVerify();

		long verify(const char* input_file, VERIFY_RESULT* verifyResult);

};


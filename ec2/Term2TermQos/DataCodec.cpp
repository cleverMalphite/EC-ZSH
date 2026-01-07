#include "DataCodec.h"

namespace Term2TermQos {
    void WriteData(BYTE *pBuffer, unsigned long dwData) {
        if (pBuffer) {
            BYTE hhByte = (BYTE) ((dwData & 0xff000000) >> 24);
            BYTE hlByte = (BYTE) ((dwData & 0x00ff0000) >> 16);
            BYTE lhByte = (BYTE) ((dwData & 0x0000ff00) >> 8);
            BYTE llByte = (BYTE) ((dwData & 0x000000ff));
            pBuffer[0] = hhByte;
            pBuffer[1] = hlByte;
            pBuffer[2] = lhByte;
            pBuffer[3] = llByte;
        }
    }

    void WriteChar(BYTE *pBuffer, BYTE *pszString, DWORD nLength) {
        if (pBuffer && pszString && nLength) {
            memcpy(pBuffer, (BYTE *) pszString, nLength);
        }
    }

    DWORD ReadData(BYTE *pBuffer) {
        DWORD dwValue = 0;
        if (pBuffer) {
            BYTE hhByte = pBuffer[0];
            BYTE hlByte = pBuffer[1];
            BYTE lhByte = pBuffer[2];
            BYTE llByte = pBuffer[3];
            dwValue = ((DWORD) (hhByte) << 24) + ((DWORD) hlByte << 16) + ((DWORD) lhByte << 8) + ((DWORD) llByte);
        }
        return dwValue;
    }

    BYTE *ReadChar(BYTE *pBuffer, DWORD nLength) {
        if (pBuffer && nLength) {
            BYTE *szValue = new BYTE[nLength];
            memcpy(szValue, (char *) pBuffer, nLength);
            return szValue;
        }
        return nullptr;
        /*std::string szValue;

        szValue.clear();
        if (pBuffer && nLength && nLength < PATH_MAX) {
            char szTemp[PATH_MAX];
            memcpy(szTemp, pBuffer, nLength);
            szTemp[nLength] = '\0';
            szValue = szTemp;
        }
        return szValue;*/
    }
}
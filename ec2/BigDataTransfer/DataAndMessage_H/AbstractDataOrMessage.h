//
// Created by 王炳棋 on 2023/1/11.
//

#ifndef NETCOMBTRANSFER_ABSTRACTDATAORMESSAGE_H
#define NETCOMBTRANSFER_ABSTRACTDATAORMESSAGE_H

#include "../mGlobalDef.h"
#include "../GlobalMessage.h"

namespace BigDataTransfer {
    class AbstractData {
    public:
        virtual std::shared_ptr<BYTE> Encode(DWORD &dDataLength) = 0;

        virtual bool Decode(std::shared_ptr<BYTE> pBuffer, DWORD dLength) = 0;
    };

    class AbstractMessage {
    public:
        virtual std::shared_ptr<BYTE> Encode(DWORD &dDataLength) = 0;

        virtual bool Decode(std::shared_ptr<BYTE> pBuffer, DWORD dLength) = 0;

        virtual DWORD GetMessageFlag() {
            return 0;
        }
    };
}

#endif //NETCOMBTRANSFER_ABSTRACTDATAORMESSAGE_H

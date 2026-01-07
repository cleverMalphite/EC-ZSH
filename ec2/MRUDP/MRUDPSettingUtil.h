//
// Created by 王炳棋 on 2022/12/31.
//

#ifndef NETCOMBTRANSFER_MRUDPSETTINGUTIL_H
#define NETCOMBTRANSFER_MRUDPSETTINGUTIL_H

#include <vector>
#include "MRUDPTerm2TermConfig.h"

namespace MRUDP {
    /**
	 * 获取(Role, Channel)的配置记录项
	 */
    std::vector<std::shared_ptr<MRUDPTerm2TermConfig>> GetSettingsForMRUDP();
}

#endif //NETCOMBTRANSFER_MRUDPSETTINGUTIL_H

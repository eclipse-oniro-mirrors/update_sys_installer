/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef SYS_INSTALLER_SERVER_H
#define SYS_INSTALLER_SERVER_H

#include <iostream>
#include <thread>
#include "if_system_ability_manager.h"
#include "sys_installer_manager.h"
#include "stream_installer_manager.h"
#include "ipc_skeleton.h"
#include "iremote_stub.h"
#include "isys_installer.h"
#include "status_manager.h"
#include "stream_status_manager.h"
#include "system_ability.h"
#include "sys_installer_common.h"
#include "sys_installer_stub.h"

namespace OHOS {
namespace SysInstaller {
class SysInstallerServer : public SystemAbility, public SysInstallerStub {
public:
    DECLARE_SYSTEM_ABILITY(SysInstallerServer);
    DISALLOW_COPY_AND_MOVE(SysInstallerServer);
    SysInstallerServer(int32_t systemAbilityId, bool runOnCreate = false);
    ~SysInstallerServer() override;

    int32_t SysInstallerInit(const std::string &taskId, bool bStreamUpgrade) override;
    int32_t StartUpdatePackageZip(const std::string &taskId, const std::string &pkgPath) override;
    int32_t StartStreamUpdate() override;
    int32_t StopStreamUpdate() override;
    int32_t ProcessStreamData(const BufferInfoParcel &bufferParcel) override;
    int32_t SetUpdateCallback(const std::string &taskId, const sptr<ISysInstallerCallback> &updateCallback) override;
    int32_t GetUpdateStatus(const std::string &taskId) override;
    int32_t StartUpdateParaZip(const std::string &taskId, const std::string &pkgPath,
        const std::string &location, const std::string &cfgDir) override;
    int32_t StartDeleteParaZip(const std::string &taskId, const std::string &location,
        const std::string &cfgDir) override;
    int32_t AccDecompressAndVerifyPkg(const std::string &taskId, const std::string &srcPath,
        const std::string &dstPath, const uint32_t type) override;
    int32_t AccDeleteDir(const std::string &taskId, const std::string &dstPath) override;
    int32_t StartUpdateVabPackageZip(const std::string &taskId, const std::vector<std::string> &pkgPath) override;
    int32_t CancelUpdateVabPackageZip(const std::string &taskId) override;
    int32_t StartVabMerge(const std::string &taskId) override;
    int32_t CreateVabSnapshotCowImg(const std::unordered_map<std::string, uint64_t> &partitionInfo) override;
    int32_t EnableVabCheckpoint() override;
    int32_t AbortVabActiveSnapshot() override;
    int32_t ClearVabMetadataAndCow() override;
    int32_t MergeRollbackReasonFile() override;
    int32_t GetUpdateResult(const std::string &taskId, const std::string &taskType,
        const std::string &resultType, std::string &updateResult) override;
    int32_t GetMetadataUpdateStatus(int32_t &metadataStatus) override;
    int32_t VabUpdateActive() override;
    int32_t GetMetadataResult(const std::string &action, bool &result) override;
    int32_t ExitSysInstaller() override;
    int32_t StartAbSync() override;

#ifndef UPDATER_UT
private:
#else
public:
#endif
    void OnStart() override;
    void OnStop() override;

private:
    bool logInit_ = false;
    bool bStreamUpgrade_ = false;
    std::mutex sysInstallerServerLock_;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_SERVER_H

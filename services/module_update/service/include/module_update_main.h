/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef SYS_INSTALLER_MODULE_UPDATE_MAIN_H
#define SYS_INSTALLER_MODULE_UPDATE_MAIN_H

#include <unordered_map>
#include <unordered_set>

#include "imodule_update.h"
#include "iservice_registry.h"
#include "isys_installer_callback.h"
#include "module_ipc_helper.h"
#include "singleton.h"

namespace OHOS {
namespace SysInstaller {
class ModuleUpdateMain final : public Singleton<ModuleUpdateMain> {
    DECLARE_SINGLETON(ModuleUpdateMain);
public:
    bool CheckBootComplete() const;
    int32_t UninstallModulePackage(const std::string &hmpName);
    int32_t GetModulePackageInfo(const std::string &hmpName,
        std::list<ModulePackageInfo> &modulePackageInfos);
    int32_t ReportModuleUpdateStatus(const ModuleUpdateStatus &status);
    std::vector<HmpVersionInfo> GetHmpVersionInfo();
    void ExitModuleUpdate();
    int32_t InstallModuleFile(const std::string &hmpName, const std::string &file) const;
    void CollectModulePackageInfo(const std::string &hmpName, std::list<ModulePackageInfo> &modulePackageInfos) const;
    bool BackupActiveModules() const;
    bool GetHmpVersion(const std::string &hmpPath, HmpVersionInfo &versionInfo);
    void SaveInstallerResult(const std::string &hmpPath, int result, const std::string &resultInfo);
    int32_t ReallyInstallModulePackage(const std::string &pkgPath, const sptr<ISysInstallerCallback> &updateCallback);
    void ParseHmpVersionInfo(std::vector<HmpVersionInfo> &versionInfos, const HmpVersionInfo &preInfo,
        const HmpVersionInfo &actInfo);

    void ScanPreInstalledHmp();
    void Start();
    void Stop();

private:
    void BuildSaIdHmpMap(std::unordered_map<int32_t, std::string> &saIdHmpMap);
    sptr<ISystemAbilityManager> &GetSystemAbilityManager();

    sptr<ISystemAbilityManager> samgr_ = nullptr;
    std::unordered_set<std::string> hmpSet_;
    std::unordered_map<int32_t, std::string> saIdHmpMap_;
    std::unordered_map<std::string, std::unordered_set<std::string>> processHmpMap_;
};
} // namespace SysInstaller
} // namespace OHOS
#endif // SYS_INSTALLER_MODULE_UPDATE_MAIN_H
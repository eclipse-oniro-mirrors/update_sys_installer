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

#include "module_file_repository.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "directory_ex.h"
#include "log/log.h"
#include "module_constants.h"
#include "module_error_code.h"
#include "scope_guard.h"
#include "unique_fd.h"

namespace OHOS {
namespace SysInstaller {
using namespace Updater;
using std::string;

ModuleFileRepository::~ModuleFileRepository()
{
    Clear();
}

void ModuleFileRepository::InitRepository(const string &hmpName, const Timer &timer)
{
    string allPath[] = {MODULE_PREINSTALL_DIR, UPDATE_INSTALL_DIR, UPDATE_ACTIVE_DIR};
    auto& fileMap = moduleFileMap_[hmpName];
    for (string &path : allPath) {
        std::vector<string> files;
        const string checkDir = path + "/" + hmpName;
        GetDirFiles(checkDir, files);
        for (string &file : files) {
            if (!CheckFileSuffix(file, MODULE_PACKAGE_SUFFIX)) {
                continue;
            }
            // verifi zip before open it.
            if (path != MODULE_PREINSTALL_DIR && VerifyModulePackageSign(file) != 0) {
                LOG(ERROR) << "VerifyModulePackageSign failed of " << file;
                SaveInstallerResult(path, hmpName, ModuleErrorCode::ERR_VERIFY_FAIL, "verify fail", timer);
                continue;
            }
            ProcessFile(hmpName, path, file, fileMap, timer);
        }
    }
    LOG(INFO) << "InitRepository all timer:" << timer;
}

void ModuleFileRepository::SaveInstallerResult(const std::string &fpInfo, const std::string &hmpName,
    int result, const std::string &resultInfo, const Timer &timer) const
{
    if (fpInfo.find(UPDATE_INSTALL_DIR) == std::string::npos &&
        fpInfo.find(UPDATE_ACTIVE_DIR) == std::string::npos) {
        return;
    }
    if (!CheckFileSuffix(fpInfo, MODULE_PACKAGE_SUFFIX) &&
        !CheckFileSuffix(fpInfo, MODULE_IMAGE_SUFFIX)) {
        LOG(WARNING) << "SaveInstallerResult path:" << fpInfo << "; break;";
        return;
    }
    if (!CheckPathExists(MODULE_RESULT_PATH)) {
        LOG(ERROR) << MODULE_RESULT_PATH << " not exist";
        return;
    }
    LOG(INFO) << "path:" << fpInfo << "hmp:" << hmpName << "result:" << result << "Info:" << resultInfo << "\n";

    UniqueFd fd(open(MODULE_RESULT_PATH, O_APPEND | O_RDWR | O_CLOEXEC));
    if (fd.Get() == -1) {
        LOG(ERROR) << "Failed to open file";
        return;
    }

    std::string writeInfo = hmpName + ";" + std::to_string(result) + ";" +
        resultInfo + "|" + std::to_string(timer.duration().count()) + "\n";
    if (CheckAndUpdateRevertResult(hmpName, writeInfo, "mount fail")) {
        return;
    }
    if (write(fd, writeInfo.data(), writeInfo.length()) <= 0) {
        LOG(WARNING) << "write result file failed, err:" << errno;
    }
    fsync(fd.Get());
}

void ModuleFileRepository::ProcessFile(const string &hmpName, const string &path, const string &file,
    std::unordered_map<std::string, ModuleFile> &fileMap, const Timer &timer) const
{
    std::unique_ptr<ModuleFile> moduleFile = ModuleFile::Open(file);
    if (moduleFile == nullptr || moduleFile->GetVersionInfo().hmpName != hmpName) {
        return;
    }
    if (!moduleFile->GetImageStat().has_value()) {
        LOG(ERROR) << "verify failed, img is empty: " << file;
        SaveInstallerResult(path, hmpName, ModuleErrorCode::ERR_VERIFY_FAIL, "img empty", timer);
        return;
    }
    if (path != MODULE_PREINSTALL_DIR) {
        if (!CheckFilePath(*moduleFile, path)) {
            LOG(ERROR) << "Open " << file << " failed";
            SaveInstallerResult(path, hmpName, ModuleErrorCode::ERR_VERIFY_FAIL, "get pub key fail", timer);
            return;
        }
    }
    LOG(INFO) << "ProcessFile " << file << " successful";
    fileMap.insert(std::make_pair(path, std::move(*moduleFile)));
}

std::unique_ptr<ModuleFile> ModuleFileRepository::GetModuleFile(const std::string &pathPrefix,
    const string &hmpName) const
{
    auto mapIter = moduleFileMap_.find(hmpName);
    if (mapIter == moduleFileMap_.end()) {
        LOG(ERROR) << "Invalid path hmpName= " << hmpName;
        return nullptr;
    }
    std::unordered_map<std::string, ModuleFile> fileMap = mapIter->second;
    auto fileIter = fileMap.find(pathPrefix);
    if (fileIter == fileMap.end()) {
        LOG(INFO) << hmpName << " not found in " << pathPrefix;
        return nullptr;
    }
    ModuleFile file = fileIter->second;
    return std::make_unique<ModuleFile>(std::move(file));
}

bool ModuleFileRepository::IsPreInstalledModule(const ModuleFile &moduleFile) const
{
    std::unique_ptr<ModuleFile> preInstalledModule = GetModuleFile(MODULE_PREINSTALL_DIR,
        moduleFile.GetVersionInfo().hmpName);
    if (preInstalledModule == nullptr) {
        return false;
    }
    return preInstalledModule->GetPath() == moduleFile.GetPath();
}

bool ModuleFileRepository::CheckFilePath(const ModuleFile &moduleFile, const string &prefix) const
{
    std::unique_ptr<ModuleFile> preInstalledModule = GetModuleFile(MODULE_PREINSTALL_DIR,
        moduleFile.GetVersionInfo().hmpName);
    if (preInstalledModule == nullptr) {
        return false;
    }
    string prePath = preInstalledModule->GetPath();
    string curPath = moduleFile.GetPath();
    return prePath.substr(strlen(MODULE_PREINSTALL_DIR), prePath.length()) ==
        curPath.substr(prefix.length(), curPath.length());
}

void ModuleFileRepository::Clear()
{
    for (auto mapIter = moduleFileMap_.begin(); mapIter != moduleFileMap_.end(); ++mapIter) {
        std::unordered_map<std::string, ModuleFile> &fileMap = mapIter->second;
        for (auto fileIter = fileMap.begin(); fileIter != fileMap.end(); ++fileIter) {
            fileIter->second.ClearVerifiedData();
        }
        fileMap.clear();
    }
    moduleFileMap_.clear();
}

const std::unordered_map<string, std::unordered_map<string, ModuleFile>> &ModuleFileRepository::GetModuleMap(void)
{
    return moduleFileMap_;
}
} // SysInstaller
} // namespace OHOS

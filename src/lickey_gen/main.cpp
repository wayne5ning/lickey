#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <FeatureInfo.h>
#include <FileUtility.h>
#include <Date.h>
#include <Log.h>
#include <License.h>
#include <LicenseManager.h>
#include <Version.h>

#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"

using namespace lickey;

namespace
{
    void ToLower(std::string& str)
    {
        std::transform(str.begin(), str.end(), str.begin(), tolower);
    }


    void ToLowerAndTrim(std::string& str)
    {
        std::transform(str.begin(), str.end(), str.begin(), tolower);
        boost::trim(str);
    }


    bool AddFeature(
        const std::string& featureName,
        const std::string& featureVersion,
        const Date& issue,
        const std::string& expireDate,
        const unsigned int numLics,
        License& lic,
        LicenseManager& licMgr)
    {
        Date expire;
        if(!::Load(expire, expireDate))
        {
            std::cout << "invalid expire date = " << expireDate << "\n";
            return false;
        }

        FeatureVersion version;
        version = featureVersion;
        licMgr.Add(featureName, version, issue, expire, numLics, lic);
        std::cout << "done to add feature = " << featureName << "\n";
        std::cout << "  version = " << featureVersion << "\n";
        std::cout << "  issueDate date = " << ToString(issue) << "\n";
        std::cout << "  expire date = " << expireDate << "\n";
        std::cout << "  num licenses = " << numLics << "\n";
        return true;
    }
}


int batch_mode(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Invalid input. Correct: lickey_gen.exe {file} {expire_date:YYYYMMDD format}\n";
        return -1;
    }

    const auto* fileName = argv[1];
    const auto* expireDateStr = argv[2];
    const std::filesystem::path inputFile{ fileName };

    std::ifstream ifs{ fileName };
    if (ifs.bad()) {
        std::cerr << "Fail to read file: " << fileName << "\n";
        return -1;
    }
    using namespace rapidjson;
    rapidjson::Document d{};
    const auto data = std::string((std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());
    d.Parse(data.c_str());
    if (d.HasParseError()) {
        std::cerr << "Fail to parse file: " << fileName << "\n";
        return -1;
    }

    Date issueDate;
    SetToday(issueDate);
    Date expireDate;
    if (!Load(expireDate, expireDateStr))
    {
        std::cerr << "invalid date format: " << expireDateStr << "\n";
        return -1;
    }

    const auto* mac = d["mac"].GetString();
 
    const auto* venderName = d["vender_name"].GetString();
    const auto* appName = d["app_name"].GetString();
    LicenseManager licMgr(venderName, appName);
    License lic;
    for (const auto& feat: d["features"].GetArray()) {
        const auto* name = feat["name"].GetString();
        const auto* version = feat["version"].GetString();
        const auto numLics = feat["num_lics"].GetInt();
        if (!AddFeature(name, version, issueDate, expireDateStr, numLics, lic, licMgr)) {
            std::cerr << "Fail to add feature name=" << name << ", version=" << version << ", num_lics=" << numLics << "\n";
            return -1;
        }
    }

    const auto outFile = std::format("output\\{}.{}.{}.lic", inputFile.stem().string(), mac, expireDateStr);

    if (!licMgr.Save(outFile, lickey::HardwareKey{ mac }, lic)) {
        std::cerr << "Fail to write to file: " << outFile << "\n";
        return -1;
    }


    return 0;
}


int interactive_mode(int argc, char* argv[])
{
    std::cout << "License generator V" << VERSION() << "\n";
    std::cout << "(half-width characters only / without space and tabspace)\n";
    std::cout << "\n";

    std::string venderName;
    std::cout << "vender name:";
    std::cin >> venderName;
    boost::trim(venderName);

    std::string appName;
    std::cout << "application name:";
    std::cin >> appName;
    boost::trim(appName);

    std::string hardwareKey;
    std::cout << "hardware key(11-22-33-AA-BB-CC format):";
    std::cin >> hardwareKey;
    boost::trim(hardwareKey);

    LicenseManager licMgr(venderName, appName);
    License lic;
    do
    {
        std::cout << "feature name (\"quit\" to quit this operation):";
        std::string feature;
        std::cin >> feature;
        ToLowerAndTrim(feature);
        if(0 == feature.compare("quit"))
        {
            break;
        }

        std::string featureVersion;
        std::cout << "feature version(positive integer):";
        std::cin >> featureVersion;

        Date issue;
        SetToday(issue);

        std::string expireDate;
        do
        {
            std::cout << "expire date(YYYYMMDD format):";
            std::cin >> expireDate;
            ToLowerAndTrim(expireDate);
            if(0 == expireDate.compare("quit"))
            {
                break;
            }
            Date tmp;
            if(!Load(tmp, expireDate))
            {
                std::cout << "invalid date format\n";
                continue;
            }
            break;
        } while(true);

        unsigned int numLics = 0;
        do
        {
            std::cout << "num licenses(position integer):";
            std::cin >> numLics;
            if(0 == numLics)
            {
                std::cout << "num licenses must be more than 0\n";
                continue;
            }
            break;
        } while(true);

        if(!AddFeature(feature, featureVersion, issue, expireDate, numLics, lic, licMgr))
        {
            std::cout << "fail to add new feature\n";
        }
    } while(true);

    if(lic.FeatureMap().empty())
    {
        std::cout << "no feature defined\n";
        std::string buf;
        std::cin >> buf;
        return 0;
    }

    do
    {
        std::string filepath;
        std::cout << "license file name:";
        std::cin >> filepath;
        ToLowerAndTrim(filepath);
        if(0 == filepath.compare("quit"))
        {
            std::cout << "done without saving license file\n";
            break;
        }

        std::string baseFilepath = GetBaseFilePath(filepath);
        std::string extension = GetExtension(filepath);
        std::stringstream filepathImpl;
        filepathImpl << baseFilepath << "(" << hardwareKey << ")" << extension;

        if(!licMgr.Save(filepathImpl.str(), HardwareKey(hardwareKey), lic))
        {
            std::cout << "fail to save into = " << filepathImpl.str() << "\n";
        }
        else
        {
            std::cout << "done to save into = " << filepathImpl.str() << "\n";
            break;
        }
    } while(true);

    std::cout << "please press any key\n";
    std::string buf;
    std::cin >> buf;
    return 0;
}

int main(int argc, char* argv[])
{
    return batch_mode(argc, argv);
}
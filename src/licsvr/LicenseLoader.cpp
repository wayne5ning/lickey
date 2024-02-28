#include "LicenseLoader.h"

LicenseLoader::LicenseLoader()
{

}

bool LicenseLoader::isValid(const std::string& venderApp, const std::string& feature)
{
  if (const auto it = venderApp2licenses.find(venderApp); it != venderApp2licenses.cend()) {
    return it->second.FeatureMap().IsExist(feature)
      && it->second.FeatureMap().IsValid(feature)
      && it->second.FeatureMap().IsExpired(feature);
  }
  return false;
}

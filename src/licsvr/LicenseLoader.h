#pragma once

#include <string>
#include <unordered_map>

#include "LicenseManager.h"

class LicenseLoader  final
{
public:
  LicenseLoader();
  bool isValid(const std::string& venderApp, const std::string& feature);

private:
  // for each venderApp, there is only 1 license for a device
  std::unordered_map<std::string, lickey::License> venderApp2licenses{};
};


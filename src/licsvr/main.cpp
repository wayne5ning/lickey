
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <string_view>

#include "HardwareKeyGetter.h"
#include "LicenseManager.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/cfg/argv.h"
#include "httplib.h"

int main(int argc, const char* argv[]) {

  const auto port = [&]()->int {
    for (auto i = 1; i < argc; i++) {
      if (const auto str = std::string_view{ argv[i] }; str.find("port=") == 0) {
        return std::stoi(str.substr(5).data());
      }
    }
    // 8080 would trigger Trojan:Win32/Sabsik.RD.A!ml in Windows Security in Win11 24H2 26058.1400
    return 27000;
    
  }();

  spdlog::flush_every(std::chrono::seconds(3));

  spdlog::cfg::load_argv_levels(argc, argv);

  spdlog::set_default_logger([]()->std::shared_ptr<spdlog::logger> {
    auto max_size = 1048576 * 5;
    auto max_files = 3;
    if (!std::filesystem::exists("logs")) {
      std::error_code ec{};
      if (!std::filesystem::create_directories("logs", ec)) {
        return nullptr;
      }
    }

    const auto now = std::chrono::system_clock::now();
    std::stringstream ss{};
    ss << now;
    const auto date = ss.str().substr(0, 10);

    return spdlog::rotating_logger_mt("licsvr", std::format("logs/rotating_{}.txt", date), max_size, max_files);
  } ());

  using namespace httplib;

  lickey::HardwareKeyGetter keyGetter;
  lickey::HardwareKeys keys = keyGetter();

  Server svr{};

  svr.set_logger([&](const Request& req, const Response& res) {

    spdlog::debug("req.method={}, req.path={}", req.method, req.path);
    spdlog::debug("res.body={}", res.body);
  });

  svr.set_error_handler([](const Request& req, Response& res) {
    res.set_content(std::format("<p>Error Status: <span style='color:red;'>{}</span></p>", res.status), "text/html");
  });

  svr.Get("/hello", [&](const Request& req, Response& res) {
    res.set_content("World", "text/plain");
  });

  {
    lickey::LicenseManager licMgr("wuqi", "StockApp");
    lickey::License lic;
    licMgr.Load("test(CC-E1-D5-41-21-D6).txt", keys.front(), lic);



    //BOOST_CHECK_EQUAL(false, lic.FeatureMap().IsExpired("base"));
    //BOOST_CHECK_EQUAL(true, lic.FeatureMap().IsValid("base"));
  }

  svr.Get("/verify", [&](const Request& req, Response& res) {
    //if (req.params) {
    //
    //}

    res.status = 404;
  });

  spdlog::info("Server starting on port {}", port);

  if (!svr.listen("localhost", port)) {
    std::cerr << "Fail to start server on port " << port << "\n";
    return -1;
  }

  std::cout << "Quit gracefully\n";
  return 0;
}
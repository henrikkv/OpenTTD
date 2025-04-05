#ifndef METAL_API_H
#define METAL_API_H

#include <string>
#include <vector>
#include <memory>
#include <curl/curl.h>
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

struct TokenInfo {
    std::string id;
    std::string address;
    std::string name;
    std::string symbol;
    uint64_t totalSupply;
    uint64_t startingAppSupply;
    uint64_t remainingAppSupply;
    uint64_t merchantSupply;
    std::string merchantAddress;
    double price;
};

class MetalAPI {
public:
    static bool LoadEnvFile(const std::string& path = ".env");
    static std::string GetEnvVar(const std::string& name);
    static std::vector<TokenInfo> GetMerchantTokens(const std::string& api_key, const std::string& merchant_address);
    static bool CreateLiquidity(const std::string& api_key, const std::string& token_address);
    static std::string CreateToken(const std::string& api_key, const std::string& name, const std::string& symbol, const std::string& merchant_address);
    static std::string GetTokenCreationStatus(const std::string& api_key, const std::string& job_id);

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static std::map<std::string, std::string> env_vars;
};

#endif // METAL_API_H 
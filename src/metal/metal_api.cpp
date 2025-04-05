#include "metal_api.h"
#include "../debug.h"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <fstream>
#include <regex>

std::map<std::string, std::string> MetalAPI::env_vars;

bool MetalAPI::LoadEnvFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Debug(net, 0, "[metal] Failed to open .env file at {}", path);
        return false;
    }

    std::string line;
    std::regex env_re(R"(^\s*([^=\s]+)\s*=\s*(.*)$)");
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        std::smatch match;
        if (std::regex_match(line, match, env_re)) {
            std::string key = match[1];
            std::string value = match[2];
            
            // Remove quotes if present
            if (value.size() >= 2 && (value[0] == '"' || value[0] == '\'')) {
                value = value.substr(1, value.size() - 2);
            }
            
            env_vars[key] = value;
            Debug(net, 6, "[metal] Loaded env var: {}={}", key, value);
        }
    }

    return true;
}

std::string MetalAPI::GetEnvVar(const std::string& name) {
    auto it = env_vars.find(name);
    if (it != env_vars.end()) {
        return it->second;
    }
    
    // Fall back to system environment variables
    const char* value = std::getenv(name.c_str());
    return value ? value : "";
}

size_t MetalAPI::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::vector<TokenInfo> MetalAPI::GetMerchantTokens(const std::string& apiKey, const std::string& merchantAddress) {
    std::vector<TokenInfo> tokens;
    CURL* curl = curl_easy_init();
    
    if (!curl) {
        Debug(net, 0, "[metal] Failed to initialize CURL");
        return tokens;
    }

    std::string readBuffer;
    std::string url = "https://api.metal.build/merchant/all-tokens";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    struct curl_slist* headers = nullptr;
    std::string authHeader = "x-api-key: " + apiKey;
    headers = curl_slist_append(headers, authHeader.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        Debug(net, 0, "[metal] Failed to get merchant tokens: {}", curl_easy_strerror(res));
    } else {
        rapidjson::Document d;
        if (d.Parse(readBuffer.c_str()).HasParseError()) {
            Debug(net, 0, "[metal] JSON parse error: {}", rapidjson::GetParseError_En(d.GetParseError()));
        } else {
            if (d.IsArray()) {
                for (const auto& token : d.GetArray()) {
                    if (token.IsObject() && 
                        token.HasMember("merchantAddress") && 
                        token["merchantAddress"].IsString() &&
                        token["merchantAddress"].GetString() == merchantAddress) {
                        
                        TokenInfo info;
                        if (token.HasMember("id") && token["id"].IsString()) 
                            info.id = token["id"].GetString();
                        if (token.HasMember("address") && token["address"].IsString()) 
                            info.address = token["address"].GetString();
                        if (token.HasMember("name") && token["name"].IsString()) 
                            info.name = token["name"].GetString();
                        if (token.HasMember("symbol") && token["symbol"].IsString()) 
                            info.symbol = token["symbol"].GetString();
                        if (token.HasMember("totalSupply") && token["totalSupply"].IsUint64()) 
                            info.totalSupply = token["totalSupply"].GetUint64();
                        if (token.HasMember("startingAppSupply") && token["startingAppSupply"].IsUint64()) 
                            info.startingAppSupply = token["startingAppSupply"].GetUint64();
                        if (token.HasMember("remainingAppSupply") && token["remainingAppSupply"].IsUint64()) 
                            info.remainingAppSupply = token["remainingAppSupply"].GetUint64();
                        if (token.HasMember("merchantSupply") && token["merchantSupply"].IsUint64()) 
                            info.merchantSupply = token["merchantSupply"].GetUint64();
                        if (token.HasMember("merchantAddress") && token["merchantAddress"].IsString()) 
                            info.merchantAddress = token["merchantAddress"].GetString();
                        if (token.HasMember("price") && token["price"].IsNumber()) 
                            info.price = token["price"].GetDouble();
                        
                        tokens.push_back(info);
                    }
                }
            }
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return tokens;
}

bool MetalAPI::CreateLiquidity(const std::string& apiKey, const std::string& tokenAddress) {
    CURL* curl = curl_easy_init();
    
    if (!curl) {
        Debug(net, 0, "[metal] Failed to initialize CURL");
        return false;
    }

    std::string readBuffer;
    std::string url = "https://api.metal.build/token/" + tokenAddress + "/liquidity";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string authHeader = "x-api-key: " + apiKey;
    headers = curl_slist_append(headers, authHeader.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    bool success = false;

    if (res != CURLE_OK) {
        Debug(net, 0, "[metal] Failed to create liquidity: {}", curl_easy_strerror(res));
    } else {
        rapidjson::Document d;
        if (d.Parse(readBuffer.c_str()).HasParseError()) {
            Debug(net, 0, "[metal] JSON parse error: {}", rapidjson::GetParseError_En(d.GetParseError()));
        } else {
            if (d.IsObject() && d.HasMember("success") && d["success"].IsBool()) {
                success = d["success"].GetBool();
            }
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return success;
}

std::string MetalAPI::CreateToken(const std::string& api_key, const std::string& name, const std::string& symbol, const std::string& merchant_address) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("x-api-key: " + api_key).c_str());

    // Create JSON payload
    rapidjson::Document json;
    json.SetObject();
    json.AddMember("name", rapidjson::Value(name.c_str(), json.GetAllocator()), json.GetAllocator());
    json.AddMember("symbol", rapidjson::Value(symbol.c_str(), json.GetAllocator()), json.GetAllocator());
    json.AddMember("merchantAddress", rapidjson::Value(merchant_address.c_str(), json.GetAllocator()), json.GetAllocator());
    json.AddMember("canDistribute", true, json.GetAllocator());
    json.AddMember("canLP", true, json.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json.Accept(writer);
    std::string json_str = buffer.GetString();

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.metal.build/merchant/create-token");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return "";

    // Parse response to get job ID
    rapidjson::Document resp_json;
    resp_json.Parse(response.c_str());
    if (resp_json.HasMember("jobId")) {
        return resp_json["jobId"].GetString();
    }

    return "";
}

std::string MetalAPI::GetTokenCreationStatus(const std::string& api_key, const std::string& job_id) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("x-api-key: " + api_key).c_str());

    std::string url = "https://api.metal.build/merchant/create-token/status/" + job_id;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return "";

    return response;
} 
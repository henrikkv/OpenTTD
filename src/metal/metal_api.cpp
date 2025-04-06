/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file metal_api.cpp Implementation of the Metal API functionality. */

#include "metal_api.h"
#include "../debug.h"
#include "../company_base.h"
#include "../console_func.h"
#include "../strings_func.h"
#include "../misc_cmd.h"
#include "../command_func.h"
#include "../timer/timer_game_calendar.h"
#include "../table/strings.h"

#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>
#include <cstdlib>
#include <thread>
#include <chrono>

/* Static member initialization */
std::atomic<bool> MetalAPI::_initialized(false);
std::atomic<bool> MetalAPI::_task_running(false);
std::mutex MetalAPI::_api_mutex;

/**
 * Initialize CURL library and other resources needed by Metal API.
 * This function is thread-safe.
 * @return True if initialization was successful, false otherwise.
 */
bool MetalAPI::Initialize()
{
    /* Thread safety check - prevent double initialization */
    std::lock_guard<std::mutex> lock(_api_mutex);
    
    if (_initialized) return true;
    
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        Debug(net, 0, "[Metal] Failed to initialize CURL: {}", curl_easy_strerror(res));
        return false;
    }
    
    _initialized = true;
    Debug(net, 3, "[Metal] API initialized");
    return true;
}

/**
 * Clean up resources used by Metal API.
 * This function is thread-safe.
 */
void MetalAPI::Shutdown()
{
    std::lock_guard<std::mutex> lock(_api_mutex);
    
    if (!_initialized) return;
    
    curl_global_cleanup();
    _initialized = false;
    Debug(net, 3, "[Metal] API shut down");
}

/**
 * Get the value of an environment variable.
 * @param name The name of the environment variable.
 * @return The value of the environment variable, or empty string if not found.
 */
std::string MetalAPI::GetEnvVar(const std::string& name)
{
    const char* value = std::getenv(name.c_str());
    if (value == nullptr) {
        Debug(net, 0, "[Metal] Environment variable {} not found", name);
        return "";
    }
    Debug(net, 6, "[Metal] Found environment variable {}: {}", name, value);
    return std::string(value);
}

/**
 * Callback function for CURL to write response data.
 * @param contents The received data.
 * @param size The size of each data element.
 * @param nmemb The number of data elements.
 * @param userp User-provided pointer to a string to store the data.
 * @return The total size of the data received.
 */
size_t MetalAPI::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t total_size = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), total_size);
    return total_size;
}

/**
 * Make an HTTP request to the Metal API.
 * @param url The URL to request.
 * @param api_key The API key for authentication.
 * @param post_data Optional data to send in a POST request.
 * @param is_post Whether this is a POST request.
 * @return The response body as a string.
 */
std::string MetalAPI::MakeHttpRequest(const std::string& url, const std::string& api_key, 
                                     const std::string& post_data, bool is_post)
{
    if (!_initialized && !Initialize()) {
        return "";
    }

        CURL* curl = curl_easy_init();
    if (!curl) {
        Debug(net, 0, "[Metal] Failed to initialize CURL handle");
        return "";
    }

        std::string response;
        struct curl_slist* headers = nullptr;
    
    /* Set up common headers */
        headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("X-API-Key: " + api_key).c_str());

    /* Configure CURL handle */
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L); /* 15 second timeout */
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); /* Thread safety: don't use signals */

    /* Set method-specific options */
    if (is_post) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (!post_data.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        }
        }

    /* Perform the request */
        CURLcode res = curl_easy_perform(curl);
    
    /* Clean up */
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

    /* Handle errors */
    if (res != CURLE_OK) {
        Debug(net, 0, "[Metal] HTTP request failed: {}", curl_easy_strerror(res));
        return "";
    }

        return response;
}

/**
 * Get all tokens owned by a merchant.
 * @param api_key Metal API key for authentication.
 * @param merchant_address Blockchain address of the merchant.
 * @return A vector of TokenInfo structures.
 */
std::vector<TokenInfo> MetalAPI::GetMerchantTokens(const std::string& api_key, const std::string& merchant_address)
{
    std::vector<TokenInfo> tokens;
    std::string url = "https://api.metal.build/merchant/tokens?merchantAddress=" + merchant_address;
    
    std::string response = MakeHttpRequest(url, api_key);
    if (response.empty()) return tokens;
    
    /* Parse the JSON response */
    rapidjson::Document json;
    rapidjson::ParseResult result = json.Parse(response.c_str());
    
    if (!result) {
        Debug(net, 0, "[Metal] Failed to parse JSON: {}", rapidjson::GetParseError_En(result.Code()));
        return tokens;
    }
    
    if (!json.IsArray()) {
        Debug(net, 0, "[Metal] Unexpected JSON format: not an array");
        return tokens;
    }
    
    /* Extract token information */
    for (const auto& token : json.GetArray()) {
        try {
        TokenInfo info;
        info.id = token["id"].GetString();
        info.address = token["address"].GetString();
        info.name = token["name"].GetString();
        info.symbol = token["symbol"].GetString();
        info.totalSupply = token["totalSupply"].GetUint64();
        info.startingAppSupply = token["startingAppSupply"].GetUint64();
        info.remainingAppSupply = token["remainingAppSupply"].GetUint64();
        info.merchantSupply = token["merchantSupply"].GetUint64();
        info.merchantAddress = token["merchantAddress"].GetString();
        info.price = token["price"].GetDouble();
        tokens.push_back(info);
            
            Debug(net, 3, "[Metal] Found token: {} ({})", info.name, info.symbol);
        } catch (const std::exception& e) {
            Debug(net, 0, "[Metal] Error processing token: {}", e.what());
        }
    }

    return tokens;
}

/**
 * Create liquidity for a token.
 * @param api_key Metal API key for authentication.
 * @param token_address Blockchain address of the token.
 * @return True if liquidity was created successfully, false otherwise.
 */
bool MetalAPI::CreateLiquidity(const std::string& api_key, const std::string& token_address)
{
    /* Prepare JSON payload */
    rapidjson::Document json;
    json.SetObject();
    json.AddMember("tokenAddress", rapidjson::Value(token_address.c_str(), json.GetAllocator()), json.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json.Accept(writer);

    /* Make the API call */
    std::string response = MakeHttpRequest("https://api.metal.io/v1/liquidity", api_key, buffer.GetString(), true);
    if (response.empty()) return false;
    
    /* Parse the response */
    rapidjson::Document response_json;
    rapidjson::ParseResult result = response_json.Parse(response.c_str());
    
    if (!result) {
        Debug(net, 0, "[Metal] Failed to parse JSON: {}", rapidjson::GetParseError_En(result.Code()));
        return false;
    }
    
    /* Check for success */
    if (response_json.HasMember("success") && response_json["success"].GetBool()) {
        Debug(net, 3, "[Metal] Successfully created liquidity for token {}", token_address);
        return true;
    } else {
        if (response_json.HasMember("error") && response_json["error"].IsString()) {
            Debug(net, 0, "[Metal] Failed to create liquidity: {}", response_json["error"].GetString());
        } else {
            Debug(net, 0, "[Metal] Failed to create liquidity for unknown reason");
        }
        return false;
    }
}

/**
 * Create a new token on the Metal blockchain.
 * @param api_key Metal API key for authentication.
 * @param name Human-readable name for the token.
 * @param symbol Trading symbol for the token.
 * @param merchant_address Blockchain address of the merchant.
 * @return The job ID for the token creation, or empty string on failure.
 */
std::string MetalAPI::CreateToken(const std::string& api_key, const std::string& name, const std::string& symbol, const std::string& merchant_address)
{
    /* Prepare JSON payload */
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

    /* Make the API call */
    std::string response = MakeHttpRequest("https://api.metal.build/merchant/create-token", api_key, buffer.GetString(), true);
    if (response.empty()) return "";
    
    /* Parse the response */
    rapidjson::Document response_json;
    rapidjson::ParseResult result = response_json.Parse(response.c_str());
    
    if (!result) {
        Debug(net, 0, "[Metal] Failed to parse JSON: {}", rapidjson::GetParseError_En(result.Code()));
        return "";
    }
    
    /* Extract the job ID */
    if (response_json.HasMember("jobId") && response_json["jobId"].IsString()) {
        std::string job_id = response_json["jobId"].GetString();
        Debug(net, 3, "[Metal] Token creation started with job ID {}", job_id);
        return job_id;
    } else {
        if (response_json.HasMember("error") && response_json["error"].IsString()) {
            Debug(net, 0, "[Metal] Failed to create token: {}", response_json["error"].GetString());
        } else {
            Debug(net, 0, "[Metal] Failed to create token for unknown reason");
        }
        return "";
    }
}

/**
 * Check the status of a token creation job.
 * @param api_key Metal API key for authentication.
 * @param job_id The job ID returned by CreateToken.
 * @return JSON response containing the status information.
 */
std::string MetalAPI::GetTokenCreationStatus(const std::string& api_key, const std::string& job_id)
{
    std::string url = "https://api.metal.build/merchant/create-token/status/" + job_id;
    return MakeHttpRequest(url, api_key);
}

/**
 * Check if a background task is currently running.
 * @return True if a task is running, false otherwise.
 */
bool MetalAPI::IsTaskRunning()
{
    return _task_running.load();
}

/**
 * Start a background task to create tokens for all companies.
 * This function launches a thread that will create tokens for each company.
 * @param api_key Metal API key.
 * @param merchant_address Blockchain address of the merchant.
 * @return True if the background task was started successfully, false otherwise.
 */
bool MetalAPI::StartTokenCreationTask(const std::string& api_key, const std::string& merchant_address)
{
    /* Check if task is already running */
    if (_task_running.exchange(true)) {
        Debug(net, 0, "[Metal] A task is already running, cannot start token creation");
        return false;
    }
    
    /* Launch background thread for token creation */
    std::thread([api_key, merchant_address]() {
        Debug(net, 3, "[Metal] Starting token creation task");
        IConsolePrint(CC_DEFAULT, "[Metal] Starting token creation process...");
        
        /* Count companies to process */
        int company_count = 0;
        for (const Company *company : Company::Iterate()) {
            company_count++;
        }
        
        if (company_count == 0) {
            IConsolePrint(CC_ERROR, "[Metal] No companies found to process");
            _task_running = false;
            return;
        }
        
        IConsolePrint(CC_DEFAULT, "[Metal] Found {} companies to process", company_count);
        
        /* Process each company */
        int processed = 0;
        for (const Company *company : Company::Iterate()) {
            processed++;
            
            /* Get company name using simpler string handling */
            std::string company_name = fmt::format("Company #{}", company->index);
            std::string symbol = fmt::format("TTD{}", company->index);
            
            IConsolePrint(CC_DEFAULT, "[Metal] Processing company {} of {}: {} ({})", 
                processed, company_count, company_name, symbol);
            
            /* Create token */
            IConsolePrint(CC_DEFAULT, "[Metal] Creating token for {} with symbol {}", company_name, symbol);
            std::string job_id = MetalAPI::CreateToken(api_key, company_name, symbol, merchant_address);
            
            if (job_id.empty()) {
                IConsolePrint(CC_ERROR, "[Metal] Failed to create token for company {} - API call failed", company_name);
                continue;
            }
            
            IConsolePrint(CC_DEFAULT, "[Metal] Got job ID {} for token creation", job_id);
            
            /* Wait for token creation to complete */
            int retry_count = 0;
            bool success = false;
            do {
                retry_count++;
                IConsolePrint(CC_DEFAULT, "[Metal] Checking token status (attempt {})...", retry_count);
                
                /* Get token status */
                std::string status = MetalAPI::GetTokenCreationStatus(api_key, job_id);
                if (status.empty()) {
                    IConsolePrint(CC_ERROR, "[Metal] Failed to get token status - API call failed");
                    break;
                }
                
                /* Parse status response */
                rapidjson::Document status_json;
                status_json.Parse(status.c_str());
                
                std::string current_status = status_json["status"].GetString();
                IConsolePrint(CC_DEFAULT, "[Metal] Current status: {}", current_status);
                
                if (current_status == "success") {
                    /* Token created successfully */
                    std::string token_name = status_json["data"]["name"].GetString();
                    std::string token_address = status_json["data"]["address"].GetString();
                    
                    IConsolePrint(CC_DEFAULT, "[Metal] Successfully created token:");
                    IConsolePrint(CC_DEFAULT, "[Metal]   Name: {}", token_name);
                    IConsolePrint(CC_DEFAULT, "[Metal]   Address: {}", token_address);
                    success = true;
                    break;
                } else if (current_status == "pending") {
                    /* Token creation still in progress */
                    IConsolePrint(CC_DEFAULT, "[Metal] Token creation still in progress, waiting...");
                } else {
                    /* Unexpected status */
                    IConsolePrint(CC_ERROR, "[Metal] Unexpected status: {}", current_status);
                    break;
                }
                
                /* Wait before checking again */
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            } while (retry_count < 60); /* Up to 1 minute per token */
            
            if (!success) {
                if (retry_count >= 60) {
                    IConsolePrint(CC_ERROR, "[Metal] Gave up waiting for token creation after 60 attempts");
                }
            }
        }
        
        IConsolePrint(CC_DEFAULT, "[Metal] Finished processing {} companies", processed);
        IConsolePrint(CC_DEFAULT, "[Metal] Token creation process completed");
        
        /* Mark task as completed */
        _task_running = false;
    }).detach();
    
    return true;
}

/**
 * Start a background task to initialize the game with blockchain features.
 * This function launches a thread that will set up liquidity for existing tokens.
 * @param api_key Metal API key.
 * @param merchant_address Blockchain address of the merchant.
 * @return True if the background task was started successfully, false otherwise.
 */
bool MetalAPI::StartGameInitializationTask(const std::string& api_key, const std::string& merchant_address)
{
    /* Check if task is already running */
    if (_task_running.exchange(true)) {
        Debug(net, 0, "[Metal] A task is already running, cannot start game initialization");
        return false;
    }
    
    /* Launch background thread for game initialization */
    std::thread([api_key, merchant_address]() {
        Debug(net, 3, "[Metal] Starting game initialization task");
        IConsolePrint(CC_DEFAULT, "[Metal] Starting blockchain game initialization...");
        
        /* Get all tokens for this merchant */
        IConsolePrint(CC_DEFAULT, "[Metal] Fetching company tokens from the blockchain...");
        auto tokens = MetalAPI::GetMerchantTokens(api_key, merchant_address);
        
        if (tokens.empty()) {
            IConsolePrint(CC_ERROR, "[Metal] No tokens found for merchant. Please run 'create_tokens' first.");
            _task_running = false;
            return;
        }
        
        IConsolePrint(CC_DEFAULT, "[Metal] Found {} company tokens on the blockchain", tokens.size());
        
        /* Create liquidity for each token */
        int processed = 0;
        for (const auto& token : tokens) {
            processed++;
            IConsolePrint(CC_DEFAULT, "[Metal] Creating liquidity for token {} ({}/{})", 
                token.symbol, processed, tokens.size());
            
            if (MetalAPI::CreateLiquidity(api_key, token.address)) {
                IConsolePrint(CC_DEFAULT, "[Metal] Successfully created liquidity for token {}", token.symbol);
            } else {
                IConsolePrint(CC_ERROR, "[Metal] Failed to create liquidity for token {}", token.symbol);
            }
            
            /* Wait briefly between API calls */
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        /* Finalize game initialization */
        IConsolePrint(CC_DEFAULT, "[Metal] Unpause game and disable company name changes");
        Command<CMD_PAUSE>::Post(PauseMode::Normal, false);
        
        IConsolePrint(CC_DEFAULT, "[Metal] Game started! Company tokens are now active.");
        
        /* Mark task as completed */
        _task_running = false;
    }).detach();
    
    return true;
}

/**
 * Create a liquidity pool for a token.
 * @param api_key The Metal API key.
 * @param token_address The token contract address.
 * @return True if liquidity pool was created successfully, false otherwise.
 */
bool MetalAPI::CreateLiquidityPool(const std::string &api_key, const std::string &token_address) 
{
    std::string url = fmt::format("https://api.metal.build/token/{}/liquidity", token_address);

    CURL *curl = curl_easy_init();
    if (!curl) return false;

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, fmt::format("x-api-key: {}", api_key).c_str());

    std::string response;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    bool success = false;

    if (res == CURLE_OK) {
        rapidjson::Document doc;
        doc.Parse(response.c_str());
        
        if (doc.HasMember("success") && doc["success"].GetBool()) {
            success = true;
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return success;
} 
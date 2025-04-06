/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file metal_api.h Functions for interacting with the Metal blockchain service. */

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <future>
#include <atomic>

/**
 * Information about a token on the Metal blockchain.
 */
struct TokenInfo {
    std::string id;           ///< Unique ID of the token
    std::string address;      ///< Blockchain address of the token
    std::string name;         ///< Human-readable name
    std::string symbol;       ///< Trading symbol
    uint64_t totalSupply;     ///< Total supply of tokens
    uint64_t startingAppSupply;  ///< Initial supply allocated to the app
    uint64_t remainingAppSupply; ///< Current supply remaining in the app
    uint64_t merchantSupply;  ///< Supply allocated to the merchant
    std::string merchantAddress; ///< Address of the merchant
    double price;             ///< Current price
};

/**
 * Class providing API access to the Metal blockchain.
 */
class MetalAPI {
public:
    /**
     * Initialize the Metal API subsystem.
     * @return True if initialization succeeds, false otherwise.
     */
    static bool Initialize();
    
    /**
     * Clean up resources used by the Metal API subsystem.
     */
    static void Shutdown();
    
    /**
     * Get an environment variable's value.
     * @param name The variable name to retrieve.
     * @return The variable value, or empty string if not found.
     */
    static std::string GetEnvVar(const std::string& name);
    
    /**
     * Get all tokens owned by a merchant.
     * @param api_key Metal API key.
     * @param merchant_address Blockchain address of the merchant.
     * @return List of tokens owned by the merchant.
     */
    static std::vector<TokenInfo> GetMerchantTokens(const std::string& api_key, const std::string& merchant_address);
    
    /**
     * Create liquidity for a token.
     * @param api_key Metal API key.
     * @param token_address Blockchain address of the token.
     * @return True if liquidity was created successfully, false otherwise.
     */
    static bool CreateLiquidity(const std::string& api_key, const std::string& token_address);
    
    /**
     * Create a new token.
     * @param api_key Metal API key.
     * @param name Human-readable name for the token.
     * @param symbol Trading symbol for the token.
     * @param merchant_address Blockchain address of the merchant.
     * @return The job ID for the token creation, or empty string on failure.
     */
    static std::string CreateToken(const std::string& api_key, const std::string& name, const std::string& symbol, const std::string& merchant_address);
    
    /**
     * Check the status of a token creation job.
     * @param api_key Metal API key.
     * @param job_id The job ID returned by CreateToken.
     * @return JSON response containing the status information.
     */
    static std::string GetTokenCreationStatus(const std::string& api_key, const std::string& job_id);

    /**
     * Start a background task to create tokens for all companies.
     * This function launches a thread that will create tokens for each company.
     * @param api_key Metal API key.
     * @param merchant_address Blockchain address of the merchant.
     * @return True if the background task was started successfully, false otherwise.
     */
    static bool StartTokenCreationTask(const std::string& api_key, const std::string& merchant_address);

    /**
     * Start a background task to initialize the game with blockchain features.
     * This function launches a thread that will set up liquidity for existing tokens.
     * @param api_key Metal API key.
     * @param merchant_address Blockchain address of the merchant.
     * @return True if the background task was started successfully, false otherwise.
     */
    static bool StartGameInitializationTask(const std::string& api_key, const std::string& merchant_address);
    
    /**
     * Check if a background task is currently running.
     * @return True if a task is running, false otherwise.
     */
    static bool IsTaskRunning();

    /**
     * Create a liquidity pool for a token.
     * @param api_key The Metal API key.
     * @param token_address The token contract address.
     * @return True if liquidity pool was created successfully, false otherwise.
     */
    static bool CreateLiquidityPool(const std::string &api_key, const std::string &token_address);

private:
    static std::atomic<bool> _initialized; ///< Whether the API has been initialized
    static std::atomic<bool> _task_running; ///< Whether a background task is currently running
    static std::mutex _api_mutex; ///< Mutex to protect API calls

    /** 
     * Helper callback for handling HTTP responses.
     */
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
    /**
     * Helper function to make an HTTP request with proper headers.
     * @param url The URL to request.
     * @param api_key The Metal API key for authentication.
     * @param post_data Optional JSON data for POST requests.
     * @param is_post Whether this is a POST request.
     * @return The response body as a string.
     */
    static std::string MakeHttpRequest(const std::string& url, const std::string& api_key, 
                                       const std::string& post_data = "", bool is_post = false);
}; 
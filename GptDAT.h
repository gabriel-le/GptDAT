/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "DAT_CPlusPlusBase.h"
#include <string>
#include <iostream>
#include <thread>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;
using namespace TD;

/*
 This is a basic sample project to represent the usage of CPlusPlus DAT API.
 To get more help about these functions, look at DAT_CPlusPlusBase.h
*/

class GptDAT : public DAT_CPlusPlusBase
{
public:
	GptDAT(const OP_NodeInfo* info);
	virtual ~GptDAT();

	virtual void		getGeneralInfo(DAT_GeneralInfo*, const OP_Inputs*, void* reserved1) override;

	virtual void		execute(DAT_Output*,
								const OP_Inputs*,
								void* reserved) override;

	virtual void		setupParameters(OP_ParameterManager* manager, void* reserved1) override;

private:

	// We don't need to store this pointer, but we do for the example.
	// The OP_NodeInfo class store information about the node that's using
	// this instance of the class (like its name).
	const OP_NodeInfo*	myNodeInfo;

	// In this example this value will be incremented each time the execute()
	// function is called, then passes back to the DAT
    bool                    isCompleting;
    std::string             currentCompletion;

};

// Callback function to interpret server sent events and extract the generated tokens
static size_t write_callback(char *contents, size_t size, size_t nmemb, void *userData)
{
    size_t realsize = size * nmemb;
    std::string *completion = static_cast<std::string *>(userData);

    // Split incoming stream into json objects, separated by comma. Discards all data that is sent between json responses.
    std::string in = contents;
    std::string currentString = "";
    std::string events = "";
    int brackets = 0;
    for (const auto &c : in)
    {
        if (c == '{')
            brackets++;
        if (brackets == 0)
            continue;
        currentString += c;
        if (c == '}')
        {
            brackets--;
            if (brackets == 0)
            {
                // Check if the current event is an error.
                try
                {
                    json cur_event = json::parse(currentString);
                    if (cur_event.contains("error"))
                    {
                        std::cout << std::endl
                                  << cur_event["error"]["message"] << std::endl;
                        completion->append(cur_event["error"]["message"]);
                    }
                    else
                    {
                        events += currentString + ",";
                        currentString = "";
                    }
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error parsing JSON response : " << e.what() << std::endl;
                    return 0;
                }
            }
        }
    }
    // No json objects were found.
    if (events == "")
    {
        return realsize;
    }
    events.pop_back(); // Remove the last comma

    // Append the generated tokens to the current completion
    try
    {
        json j = json::parse("[" + events + "]");
        std::string tokens = "";
        for (json item : j)
        {
            if (item == NULL)
                continue;
            tokens += item["choices"][0]["text"].get<std::string>();
        }
        std::cout << tokens;
        completion->append(tokens);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error parsing JSON response : " << e.what() << std::endl;
        return 0;
    }
    return realsize;
}

// Request a text completion.
static void gpt_request(json request, std::string apikey, std::string &currentCompletion, bool &isCompleting)
{
    isCompleting = true;
    CURL *curl = curl_easy_init();
    CURLM *multi_handle = curl_multi_init();

    if (!curl || !multi_handle)
    {
        std::cerr << "Failed to initialize curl" << std::endl;
        return;
    }
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/completions");

    currentCompletion = "";

    // Call the write_callback function as the tokens are generated to add them to the currentCompletion
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &currentCompletion);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    // Set the request method to POST
    curl_easy_setopt(curl, CURLOPT_POST, 1);

    // Set the request body
    std::string request_body = request.dump();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request_body.length());

    // Set the request headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string auth = "Authorization: Bearer " + apikey;
    headers = curl_slist_append(headers, auth.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Execute the request to the OpenAI API
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        std::string err = curl_easy_strerror(res);
        std::cerr << "Error making request: " << err << "\n";
        currentCompletion = currentCompletion.append("Error making request: " + err);
    }
    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    isCompleting = false;
}

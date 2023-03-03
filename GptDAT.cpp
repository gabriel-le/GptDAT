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

#include "GptDAT.h"
#include <mutex>

std::mutex mtx;

// These functions are basic C function, which the DLL loader can find
// much easier than finding a C++ Class.
// The DLLEXPORT prefix is needed so the compile exports these functions from the .dll
// you are creating
extern "C"
{

DLLEXPORT
void
FillDATPluginInfo(DAT_PluginInfo *info)
{
	// Always return DAT_CPLUSPLUS_API_VERSION in this function.
	info->apiVersion = DATCPlusPlusAPIVersion;

	// The opType is the unique name for this TOP. It must start with a
	// capital A-Z character, and all the following characters must lower case
	// or numbers (a-z, 0-9)
	info->customOPInfo.opType->setString("Gpt");

	// The opLabel is the text that will show up in the OP Create Dialog
	info->customOPInfo.opLabel->setString("GPT Completion");

	// Will be turned into a 3 letter icon on the nodes
	info->customOPInfo.opIcon->setString("GPT");

	// Information about the author of this OP
	info->customOPInfo.authorName->setString("Gabriel-le");
	info->customOPInfo.authorEmail->setString("info@gabriel-le.com");

	// This DAT does not accept inputs
	info->customOPInfo.minInputs = 0;
	info->customOPInfo.maxInputs = 0;

}

DLLEXPORT
DAT_CPlusPlusBase*
CreateDATInstance(const OP_NodeInfo* info)
{
	// Return a new instance of your class every time this is called.
	// It will be called once per DAT that is using the .dll
	return new GptDAT(info);
}

DLLEXPORT
void
DestroyDATInstance(DAT_CPlusPlusBase* instance)
{
	// Delete the instance here, this will be called when
	// Touch is shutting down, when the DAT using that instance is deleted, or
	// if the DAT loads a different DLL
	delete (GptDAT*)instance;
}

};

GptDAT::GptDAT(const OP_NodeInfo* info) : myNodeInfo(info)
{
    isCompleting = false;
    currentCompletion = "";
}

GptDAT::~GptDAT()
{
}

void
GptDAT::getGeneralInfo(DAT_GeneralInfo* ginfo, const OP_Inputs* inputs, void* reserved1)
{
	// This will cause the node to cook every frame
	ginfo->cookEveryFrameIfAsked = true;
}

void
GptDAT::execute(DAT_Output* output,
							const OP_Inputs* inputs,
							void* reserved)
{
    if (!output)
        return;

    int complete = inputs->getParInt("Complete");
    int reset = inputs->getParInt("Reset");

    // Begin completion request
    if (complete == 1 && !isCompleting)
    {
        // Get the api key from an environment variable if it is not explicitly set with parameters.
        std::string apikey = inputs->getParString("Apikey");
        if (apikey.length() == 0)
        {
            apikey = std::getenv("OPENAI_API_KEY");
            if (apikey.length() == 0)
            {
                output->setOutputDataType(DAT_OutDataType::Text);
                output->setText("No API key specified");
                return;
            }
        }
        isCompleting = true;
        std::cout << "Api Key : " << apikey << std::endl;

        // The prompt is separated into three inputs to allow prompt building.
        std::string prompt = inputs->getParString("Prompt");
        std::string prefix = inputs->getParString("Prefix");
        std::string suffix = inputs->getParString("Suffix");

        json j = {
            {"prompt", prefix + prompt + suffix},
            {"stop", inputs->getParString("Stop")},
            {"model", inputs->getParString("Model")},
            {"max_tokens", inputs->getParInt("Maxtokens")},
            {"temperature", inputs->getParDouble("Temperature")},
            {"top_p", inputs->getParDouble("Topp")},
            {"presence_penalty", inputs->getParDouble("Presencepenalty")},
            {"frequency_penalty", inputs->getParDouble("Frequencypenalty")},
            {"stream", true},
        };

        // Make the API call in a separate thread. The currentCompletion string will be updated as the tokens are generated.
        std::thread gptThread(gpt_request, j, apikey, std::ref(currentCompletion), std::ref(isCompleting));
        gptThread.detach();
    }

    // Update the output with the current completion;
    mtx.lock();
    if (isCompleting)
    {
        output->setOutputDataType(DAT_OutDataType::Text);
        output->setText(currentCompletion.c_str());
    }
    mtx.unlock();

    // Clear the output
    if (reset == 1)
    {
        isCompleting = false;
        currentCompletion = "";
        output->setOutputDataType(DAT_OutDataType::Text);
        output->setText("");
    }
}

void
GptDAT::setupParameters(OP_ParameterManager* manager, void* reserved1)
{
    // OpenAI API Key
    {
        OP_StringParameter np;
        np.name = "Apikey";
        np.label = "Api Key";
        np.defaultValue = "";
        OP_ParAppendResult res = manager->appendString(np);
        assert(res == OP_ParAppendResult::Success);
    }

    // Model
    {
        OP_StringParameter np;
        np.name = "Model";
        np.label = "Model";
        np.defaultValue = "text-davinci-003";
        OP_ParAppendResult res = manager->appendString(np);
        assert(res == OP_ParAppendResult::Success);
    }

    // Prepend typed in prompt
    {
        OP_StringParameter np;
        np.name = "Prefix";
        np.label = "Prefix";
        np.defaultValue = "";
        OP_ParAppendResult res = manager->appendString(np);
        assert(res == OP_ParAppendResult::Success);
    }

    // Append typed in prompt
    {
        OP_StringParameter np;
        np.name = "Suffix";
        np.label = "Suffix";
        np.defaultValue = "";
        OP_ParAppendResult res = manager->appendString(np);
        assert(res == OP_ParAppendResult::Success);
    }
    // Prompt
    {
        OP_StringParameter np;
        np.name = "Prompt";
        np.label = "Prompt";
        np.defaultValue = "";
        OP_ParAppendResult res = manager->appendString(np);
        assert(res == OP_ParAppendResult::Success);
    }
    // Stop
    {
        OP_StringParameter np;
        np.name = "Stop";
        np.label = "Stop sequence";
        np.defaultValue = "";
        OP_ParAppendResult res = manager->appendString(np);
        assert(res == OP_ParAppendResult::Success);
    }
    // Temperature
    {
        OP_NumericParameter np;
        np.name = "Temperature";
        np.label = "Temperature";
        np.defaultValues[0] = 1;
        np.minSliders[0] = 0.0;
        np.maxSliders[0] = 1.0;
        np.minValues[0] = 0;
        np.maxValues[0] = 1;
        np.clampMins[0] = true;
        np.clampMaxes[0] = true;
        OP_ParAppendResult res = manager->appendFloat(np);
        assert(res == OP_ParAppendResult::Success);
    }
    // Top P
    {
        OP_NumericParameter np;
        np.name = "Topp";
        np.label = "Top P";
        np.defaultValues[0] = 1;
        np.minSliders[0] = 0.0;
        np.maxSliders[0] = 1.0;
        np.minValues[0] = 0;
        np.maxValues[0] = 1;
        np.clampMins[0] = true;
        np.clampMaxes[0] = true;
        OP_ParAppendResult res = manager->appendFloat(np);
        assert(res == OP_ParAppendResult::Success);
    }
    // Max Tokens
    {
        OP_NumericParameter np;
        np.name = "Maxtokens";
        np.label = "Max Tokens";
        np.defaultValues[0] = 128;
        np.minSliders[0] = 0;
        np.maxSliders[0] = 4096;
        np.minValues[0] = 0;
        np.maxValues[0] = 4096;
        np.clampMins[0] = true;
        np.clampMaxes[0] = true;
        OP_ParAppendResult res = manager->appendInt(np);
        assert(res == OP_ParAppendResult::Success);
    }
    // Presence penalty
    {
        OP_NumericParameter np;
        np.name = "Presencepenalty";
        np.label = "Presence Penalty";
        np.defaultValues[0] = 0;
        np.minSliders[0] = -2.0;
        np.maxSliders[0] = 2.0;
        np.minValues[0] = -2;
        np.maxValues[0] = 2;
        np.clampMins[0] = true;
        np.clampMaxes[0] = true;
        OP_ParAppendResult res = manager->appendFloat(np);
        assert(res == OP_ParAppendResult::Success);
    }
    // frequence penalty
    {
        OP_NumericParameter np;
        np.name = "Frequencypenalty";
        np.label = "Frequency Penalty";
        np.defaultValues[0] = 0;
        np.minSliders[0] = -2.0;
        np.maxSliders[0] = 2.0;
        np.minValues[0] = -2;
        np.maxValues[0] = 2;
        np.clampMins[0] = true;
        np.clampMaxes[0] = true;
        OP_ParAppendResult res = manager->appendFloat(np);
        assert(res == OP_ParAppendResult::Success);
    }
    // Completion pulse
    {
        OP_NumericParameter np;

        np.name = "Complete";
        np.label = "Complete";

        OP_ParAppendResult res = manager->appendPulse(np);
        assert(res == OP_ParAppendResult::Success);
    }

    // Reset pulse
    {
        OP_NumericParameter np;

        np.name = "Reset";
        np.label = "Reset";

        OP_ParAppendResult res = manager->appendPulse(np);
        assert(res == OP_ParAppendResult::Success);
    }

}

# GPT Completion Touch Operator

This DAT operator allows you to interact with OpenAI's API to generate text completion. The tokens are added to the DAT's output as they are generated.

The operator has a few parameters :

- Api Key : Your OpenAI API key. To get an API key, log in to your OpenAI account and go to the following link : https://platform.openai.com/account/api-keys

- Model : The name of the model to use for text completion

- Prefix _(optional)_ : A string that will automatically be added before the prompt. Useful for [prompt engineering](https://en.wikipedia.org/wiki/Prompt_engineering).

- Suffix _(optional)_ : A string that will automatically be added after the prompt. Useful for [prompt engineering](https://en.wikipedia.org/wiki/Prompt_engineering).

- Prompt : The text prompt for completion

- Stop sequence : A string or json array of strings that should stop the token generation should they appear.

- Temperature, Top P, Max tokens, Presence penalty, Frequency penalty : See [OpenAI's documentation](https://platform.openai.com/docs/api-reference/completions/create) for details on how to use those parameters

- Complete : Start generating with GPT-3

- Reset : Erase the DAT's output

## Requirements

To compile this project, you will need to have libcurl installed on your machine.

## Installing the built plugin

As per Derivative's documentation, the plugin files should be placed in your TouchDesigner's plugins folder.

The Windows location is:

```
Documents/Derivative/Plugins
```

Which is usually:

```
C:/Users/<username>/Documents/Derivative/Plugins
```

On macOS the location is:

```
/Users/<username>/Library/Application Support/Derivative/TouchDesigner099/Plugins
```

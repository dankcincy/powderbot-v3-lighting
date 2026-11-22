# Connect your own AI

[Guide home](index.md)

Use your own assistant account. This repository hosts the shared project documents and questions. Your assistant's plan and tool support determine its access and usage costs.

## Start with the public project

Give your assistant this URL:

https://github.com/dankcincy/powderbot-v3-lighting

Ask it to read `AGENTS.md`, `README.md`, `docs/wiring.md` and `docs/provenance.md`, then the files relevant to your question. Ask it to name the files it actually read. Browser access can vary, so verify that it has retrieved the content.

## Connect through GitHub

If your assistant offers a GitHub integration, sign into your own GitHub account through that assistant's official connection flow. Grant access appropriate to this project. First test a read by asking it to summarize the open wiring questions and cite the source file. Posting comments requires a tool with write support and your authorization.

ChatGPT offers an [official GitHub plugin](https://openai.com/business/plugins/github/). Other assistants have their own connection instructions and plan requirements. A successful repository read does not establish access to issue comments or uploaded images; test those separately.

## Download fallback

Choose **Code → Download ZIP** on GitHub and upload the needed Markdown documents and relevant source files to your assistant. Tell it the repository revision or download date. Copy any relevant issue discussion and photos as well. This is a snapshot and needs refreshing after guide updates.

## A prompt to begin

> Help me reproduce the PowderBot V3 BLE bridge and lighting setup documented in this repository. The complete merged baseline is still being recovered; the packaged v1_13 sketch is an earlier passive-lighting reference. Read AGENTS.md and the guide first. Explain one step at a time. Cite project files for wiring claims. Identify unresolved endpoints before giving connection instructions. Treat photographs and bench reports as dated evidence. Ask for clarification when the evidence cannot establish a connection. Help me write a clear GitHub issue with my question and observations.

## Working with Dan and his assistant

Post one question per GitHub issue, with the guide section and clear photos. Dan and his assistant can read and answer the shared issue when reviewing the project. The repository provides shared records; automatic notification handling or AI responses require an additional configured service.

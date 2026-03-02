const { Client } = require("@modelcontextprotocol/sdk/client/index.js");
const { StreamableHTTPClientTransport } = require("@modelcontextprotocol/sdk/client/streamableHttp.js");

async function main() {
  const token = process.env.FIGMA_MCP_TOKEN;
  if (!token) {
    throw new Error("Missing FIGMA_MCP_TOKEN environment variable.");
  }

  const toolName = process.argv[2];
  const rawArgs = process.argv[3] || process.env.MCP_TOOL_ARGS_JSON || "{}";
  if (!toolName) {
    throw new Error("Usage: node mcp-call-tool.cjs <toolName> [jsonArgs]");
  }

  let parsedArgs;
  try {
    parsedArgs = JSON.parse(rawArgs);
  } catch (err) {
    throw new Error(`Invalid JSON args: ${rawArgs}`);
  }

  const client = new Client(
    { name: "figma-mcp-local-client", version: "1.0.0" },
    { capabilities: {} }
  );

  const transport = new StreamableHTTPClientTransport(new URL("https://mcp.figma.com/mcp"), {
    requestInit: {
      headers: {
        Authorization: `Bearer ${token}`,
      },
    },
  });

  await client.connect(transport);
  const result = await client.callTool({ name: toolName, arguments: parsedArgs });
  console.log(JSON.stringify(result, null, 2));
  await transport.close();
}

main().catch((err) => {
  console.error(err && err.stack ? err.stack : err);
  process.exit(1);
});

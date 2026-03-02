const { Client } = require("@modelcontextprotocol/sdk/client/index.js");
const { StreamableHTTPClientTransport } = require("@modelcontextprotocol/sdk/client/streamableHttp.js");

async function main() {
  const token = process.env.FIGMA_MCP_TOKEN;
  if (!token) {
    throw new Error("Missing FIGMA_MCP_TOKEN environment variable.");
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

  const tools = await client.listTools();
  const target = tools.tools.find((t) => t.name === "generate_figma_design");

  console.log("TOOLS:");
  for (const tool of tools.tools) {
    console.log(`- ${tool.name}`);
  }

  console.log("\nGENERATE_FIGMA_DESIGN_SCHEMA:");
  console.log(JSON.stringify(target ? target.inputSchema : null, null, 2));

  const whoami = await client.callTool({ name: "whoami", arguments: {} });
  console.log("\nWHOAMI:");
  console.log(JSON.stringify(whoami, null, 2));

  await transport.close();
}

main().catch((err) => {
  console.error(err && err.stack ? err.stack : err);
  process.exit(1);
});

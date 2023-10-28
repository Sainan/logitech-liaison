#include <iostream>

#include <Canvas.hpp>
#include <HttpRequest.hpp>
#include <IpcSocket.hpp>
#include <main.hpp>
#include <Server.hpp>
#include <ServerWebService.hpp>
#include <string.hpp>
#include <StringRefReader.hpp>
#include <StringWriter.hpp>
#include <Thread.hpp>

using namespace soup;

static bool active = false;
static Canvas c{ 21, 6 };

static IpcSocket sock;

int entrypoint(std::vector<std::string>&& args, bool console)
{
	// HTTP Server
	Server serv;

	ServerWebService srv([](Socket& s, HttpRequest&& req, ServerWebService&)
	{
		if (req.path == "/")
		{
			std::string html = "<p>Logitech Liaison at your service.</p>";
			if (active)
			{
				html += "<p>Effect at the time of your request:</p>";
				html += c.toSvg(8);
			}
			else
			{
				html += "<p>No effect is active as of the time of your request.</p>";
			}
			html += "<p>/raw returns binary data (u8 of r, g, b for each pixel) or an empty body if no effect is active.</p>";
			ServerWebService::sendHtml(s, std::move(html));
		}
		else if (req.path == "/raw")
		{
			StringWriter sw;
			if (active)
			{
				for (auto& clr : c.pixels)
				{
					sw.u8(clr.r);
					sw.u8(clr.g);
					sw.u8(clr.b);
				}
			}
			ServerWebService::sendText(s, sw.data);
		}
		else
		{
			ServerWebService::send404(s);
		}
	});

	if (!serv.bind(49748, &srv)) // port decided by a fair dice roll :)
	{
		MessageBoxA(0, "Failed to bind TCP/49748.", "Logitech Liaison", MB_OK | MB_ICONERROR);
		return 2;
	}
	std::cout << "Listening on TCP/49748" << std::endl;

	// Pipe Server
	Thread t;
	if (sock.bind("LGS_LED_SDK-00000001"))
	{
		t.start([]
		{
			while (sock.accept())
			{
				std::cout << "accept\n";
				while (true)
				{
					auto data = sock.read();
					if (data.empty())
					{
						break;
					}
					//std::cout << "read: " << string::bin2hex(data) << "\n";
					while (true)
					{
						StringRefReader sr(data);
						if (!sr.hasMore())
						{
							break;
						}
						uint32_t len;
						sr.u32(len);
						while (len > data.size())
						{
							data.append(sock.read());
						}
						uint32_t type;
						sr.u32(type);
						// https://github.com/Artemis-RGB/Artemis.Plugins.Wrappers/blob/master/src/Logitech/Artemis.Plugins.Wrappers.Logitech/Services/Enums/LogitechPipeCommand.cs
						if (type == 0x8'01)
						{
							// Init packet, contains connecting executable's path as wstring.
						}
						else if (type == 0x8'02)
						{
							uint32_t r, g, b;
							(sr.u32(r), sr.u32(g), sr.u32(b));
							r = (r * 255 / 100);
							g = (g * 255 / 100);
							b = (b * 255 / 100);
							Rgb colour(r, g, b);
							for (auto& clr : c.pixels)
							{
								clr = colour;
							}
							active = true;
						}
						else if (type == 0x8'10)
						{
							for (auto& clr : c.pixels)
							{
								uint8_t a;
								(sr.u8(clr.b), sr.u8(clr.g), sr.u8(clr.r), sr.u8(a));
							}
							active = true;
						}
						else
						{
							std::cout << "unhandled packet type " << type;
							if (data.length() > 8)
							{
								std::cout << ": " << string::bin2hex(data.substr(8, len - 8));
							}
							std::cout << "\n";
						}
						data.erase(0, len);
					}
				}
				std::cout << "disconnect\n";
				sock.disconnect();
				active = false;
			}
		});
	}
	else
	{
		MessageBoxA(0, "Failed to bind pipe.", "Logitech Liaison", MB_OK | MB_ICONERROR);
		return 1;
	}

	serv.run();
	return 0;
}

// Using GUI type to avoid having a console window for a should-be background process.
SOUP_MAIN_GUI(entrypoint);

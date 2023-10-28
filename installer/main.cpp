#include <filesystem>
#include <fstream>

#include <main.hpp>
#include <osRegistry.hpp>
#include <string.hpp>

#include "bin.hpp"

static void placeBinFile(std::filesystem::path path, const char* data, size_t size)
{
	std::ofstream out(path, std::ios::binary | std::ios::out);
	out.write(data, size);
	if (out.bad())
	{
		throw std::exception("No permission to write");
	}
	out.close();
}

static std::filesystem::path placeLogitechDll(const char* arch, const char* data, size_t size)
{
	auto path = std::filesystem::path(_wgetenv(L"PROGRAMFILES")) / "Logitech Gaming Software" / "SDK" / "LED" / arch;
	std::filesystem::create_directories(path);
	path /= "LogitechLed.dll";
	if (!std::filesystem::is_regular_file(path))
	{
		placeBinFile(path, data, size);
	}
	return path;
}

int entrypoint(std::vector<std::string>&& args, bool console)
{
	system("taskkill /im logitech-liaison.exe /f");

	const auto startup_dir = std::filesystem::path(_wgetenv(L"appdata")) / "Microsoft" / "Windows" / "Start Menu" / "Programs" / "Startup";

	placeBinFile(startup_dir / "logitech-liaison.exe", bin::logitech_liaison, sizeof(bin::logitech_liaison));

	std::filesystem::path LogitechLed_x64, LogitechLed_x86;
	try
	{
		LogitechLed_x64 = placeLogitechDll("x64", bin::LogitechLed_x64, sizeof(bin::LogitechLed_x64));
		LogitechLed_x86 = placeLogitechDll("x86", bin::LogitechLed_x86, sizeof(bin::LogitechLed_x86));
	}
	catch (const std::exception&)
	{
		MessageBoxA(0, "Some changes that need to be made require administrator privileges.", "Logitech Liaison Installer", MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	{
		auto kClasses = soup::osRegistry::LOCAL_MACHINE.getSubkey("SOFTWARE").getSubkey("Classes");
		auto kCLSID = kClasses.getSubkey("CLSID");
		auto kCLSIDx86 = kClasses.getSubkey("WOW6432Node").getSubkey("CLSID");
		try
		{
			kCLSID.createSubkey("{a6519e67-7632-4375-afdf-caa889744403}")
				.createSubkey("ServerBinary")
				.setValue(soup::string::fixType(LogitechLed_x64.u8string()))
				;
			kCLSIDx86.createSubkey("{a6519e67-7632-4375-afdf-caa889744403}")
				.createSubkey("ServerBinary")
				.setValue(soup::string::fixType(LogitechLed_x86.u8string()))
				;
		}
		catch (const std::exception&)
		{
			MessageBoxA(0, "Some changes that need to be made require administrator privileges.", "Logitech Liaison Installer", MB_OK | MB_ICONEXCLAMATION);
			return 1;
		}
	}

#if true
	MessageBoxA(0, "Logitech Liaison was successfully installed.", "Logitech Liaison Installer", MB_OK | MB_ICONINFORMATION);

	std::wstring cmd = (startup_dir / "logitech-liaison.exe").wstring();
	cmd.push_back(L'"');
	cmd.insert(0, 1, L'"');
	cmd.insert(0, LR"(cmd /c start "deez" /B )");
	_wsystem(cmd.c_str());
#endif

	return 0;
}

SOUP_MAIN_GUI(entrypoint);

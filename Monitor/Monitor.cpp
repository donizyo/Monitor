// Monitor.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <PhysicalMonitorEnumerationAPI.h>
#include <HighLevelMonitorConfigurationAPI.h>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Dxva2.lib")

typedef void PhyMonitorCB(HANDLE, va_list);

static void HandlePhysicalMonitor(PhyMonitorCB, ...);
static PhyMonitorCB CheckMonitorBrightness;
static PhyMonitorCB ChangeMonitorBrightness;

int main(int argc, char ** argv)
{
	char * command;
	char * value;
	switch (argc)
	{
	case 1:
		// print help
		goto print_help;
	case 3:
		command = argv[1];
		value = argv[2];
		if (!strcoll("brightness", command))
		{
			DWORD brightness = (DWORD)(0xffffu & atoi(value));
			HandlePhysicalMonitor(ChangeMonitorBrightness, brightness);
			HandlePhysicalMonitor(CheckMonitorBrightness);
		}
		else
		{
			printf("Unknown command!\n");
			goto print_help;
		}
		break;
	}

    return 0;
print_help:
	printf("Usage: %s brightness <value>\n"
		"\n",
		argv[0]);
	HandlePhysicalMonitor(CheckMonitorBrightness);
	return 1;
}

static void CheckMonitorBrightness(HANDLE hPhysicalMonitor, va_list vl)
{
	DWORD min, cur, max;
	BOOL bSuccess = GetMonitorBrightness(hPhysicalMonitor, &min, &cur, &max);
	if (bSuccess)
	{
		printf("Current monitor brightness: %ld.\n", cur);
		printf("Minimal monitor brightness: %ld.\n", min);
		printf("Maximal monitor brightness: %ld.\n", max);
	}
	else
	{
		printf("ERR @ GetMonitorBrightness() = %ld\n", GetLastError());
	}
}

// 改变显示器亮度
// https://msdn.microsoft.com/en-us/library/windows/desktop/dd692972(v=vs.85).aspx
static void ChangeMonitorBrightness(HANDLE hPhysicalMonitor, va_list vl)
{
	DWORD dwNewBrightness = va_arg(vl, DWORD);
	SetMonitorBrightness(hPhysicalMonitor, dwNewBrightness);
}

static void HandlePhysicalMonitor(PhyMonitorCB callback, ...)
{
	HMONITOR hMonitor = NULL;
	DWORD cPhysicalMonitors;
	LPPHYSICAL_MONITOR pPhysicalMonitors = NULL;

	// Get the monitor handle.
	hMonitor = MonitorFromWindow((HWND) NULL, MONITOR_DEFAULTTOPRIMARY);

	assert(hMonitor);

	// Get the number of physical monitors.
	BOOL bSuccess = GetNumberOfPhysicalMonitorsFromHMONITOR(
		hMonitor,
		&cPhysicalMonitors
	);

	if (bSuccess)
	{
		printf("Number of Physical Monitors: %d\n", cPhysicalMonitors);

		// Allocate the array of PHYSICAL_MONITOR structures.
		pPhysicalMonitors = (LPPHYSICAL_MONITOR)malloc(
			cPhysicalMonitors * sizeof(PHYSICAL_MONITOR));

		if (pPhysicalMonitors != NULL)
		{
			// Get the array.
			bSuccess = GetPhysicalMonitorsFromHMONITOR(
				hMonitor, cPhysicalMonitors, pPhysicalMonitors);
			if (!bSuccess)
			{
				printf("ERR @ GetPhysicalMonitorsFromHMONITOR() = %d\n",
					GetLastError());
				goto mke_ppm;
			}

			// Use the monitor handles (not shown).
			for (DWORD i = 0; i < cPhysicalMonitors; i++)
			{
				HANDLE hPhysicalMonitor = pPhysicalMonitors[i].hPhysicalMonitor;
				va_list vl;
				va_start(vl, callback);
					callback(hPhysicalMonitor, vl);
				va_end(vl);
			}

			// Close the monitor handles.
			bSuccess = DestroyPhysicalMonitors(
				cPhysicalMonitors,
				pPhysicalMonitors);
			if (!bSuccess)
			{
				printf("ERR @ DestroyPhysicalMonitors() = %d\n",
					GetLastError());
			}
		mke_ppm:
			// Free the array.
			free(pPhysicalMonitors);
		}
		else
		{
			printf("ERR @ malloc() = %d\n",
				GetLastError());
		}
	}
	else
	{
		printf("ERR @ GetNumberOfPhysicalMonitorsFromHMONITOR() = %d\n",
			GetLastError());
	}
}

// 关闭所有显示器
// https://stackoverflow.com/questions/32847726/cpp-how-to-turn-off-specific-monitor
void KillAllMonitors()
{
	SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM)2);
}

// 处理键盘事件
// https://stackoverflow.com/questions/1437158/c-win32-keyboard-events
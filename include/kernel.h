#pragma once

void kpanic()
{
	__asm__("hlt");
}

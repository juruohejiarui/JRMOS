#include <screen/screen.h>
#include <lib/string.h>
#include <lib/spinlock.h>
#include <mm/dmas.h>

struct Position {
	int xRes, yRes, xPos, yPos;
	u32 *fbAddr;
	u32 fbLen;
} position;

static u32 *_bufAddr;
static u32 _lineLen[4096];
static u32 _lineSize;

static SpinLock _printLck, _bufLck;

screen_Info *screen_info;

void screen_init() {
	_bufAddr = NULL;
	memset(_lineLen, 0, sizeof(_lineLen));
	SpinLock_init(&_printLck);
	SpinLock_init(&_bufLck);
	
	position.xRes = screen_info->horRes & 0xffff;
	position.yRes = screen_info->verRes & 0xffff;

	position.xPos = position.yPos = 0;

	position.fbAddr = mm_dmas_phys2Virt(screen_info->frameBufBase);

	_lineSize = screen_info->pixelPreLine * screen_charHeight;
}
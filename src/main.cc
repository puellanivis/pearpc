/*
 *	PearPC
 *	main.cc
 *
 *	Copyright (C) 2003, 2004 Sebastian Biallas (sb@biallas.net)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <cstdlib>
#include <cstring>
#include <exception>
#include <unistd.h>

#include "info.h"
#include "cpu_generic/ppc_cpu.h"
#include "cpu_generic/ppc_dec.h"
#include "cpu_generic/ppc_mmu.h"
#include "cpu_generic/ppc_tools.h"
#include "debug/debugger.h"
#include "io/io.h"
#include "io/graphic/gcard.h"
#include "io/ide/ide.h"
#include "io/ide/cd.h"
#include "io/prom/prom.h"
#include "io/prom/promboot.h"
#include "io/prom/prommem.h"
#include "tools/atom.h"
#include "tools/data.h"
#include "tools/except.h"
#include "tools/snprintf.h"
#include "system/display.h"
#include "system/sys.h"
#include "configparser.h"

#include "system/gif.h"
#include "system/gui/gui.h"

#include "ppc_font.h"
#include "ppc_img.h"
#include "ppc_button_changecd.h"


/*
 *
 */
 
void changeCDFunc(void *p)
{
	int *i = (int *)p;
	IDEConfig *idecfg = ide_get_config(*i);
	
	CDROMDevice *dev = (CDROMDevice *)idecfg->device;
	
	dev->acquire();
	
	if (dev->isLocked()) {
		dev->release();
		
		// sys_gui_messagebox("cdrom is locked!");
	} else {
		dev->setReady(false);
		dev->release();
		/*
		 * because we have set ready to false, no one can use
		 * the cdrom now (no medium present)
		 */
		String fn;
		if (sys_gui_open_file_dialog(fn, "title", "*.*", "alle", "testa", true)) {
			dev->acquire();
			((CDROMDeviceFile *)dev)->changeDataSource(fn);
			dev->setReady(true);
			dev->release();
		} else {
			/*
			 * the user picked no file / canceled the dialog.
			 * what's better now, to leave the old medium
			 * or to set no medium present?
			 * we choose the second option.
			 */
		}
	}
}

void initMenu()
{
	IDEConfig *idecfg = ide_get_config(0);
	if (idecfg->installed && idecfg->protocol == IDE_ATAPI) {
		MemMapFile changeCDButton(ppc_button_changecd, sizeof ppc_button_changecd);
		int *i = new int;
		*i = 0;
		gDisplay->insertMenuButton(changeCDButton, changeCDFunc, i);
	}
	idecfg = ide_get_config(1);
	if (idecfg->installed && idecfg->protocol == IDE_ATAPI) {
		MemMapFile changeCDButton(ppc_button_changecd, sizeof ppc_button_changecd);
		int *i = new int;
		*i = 1;
		gDisplay->insertMenuButton(changeCDButton, changeCDFunc, i);
	}
	gDisplay->finishMenu();
}

static char *textlogo UNUSED = "\e[?7h\e[40m\e[2J\e[40m\n\n\n\n\n\e[0;1m"
"\e[24C\xda\xc4\xc4\xc4\xc4\xc4\xc4\xc4  "
"\xda\xc4\xc4\xc4\xc4\xc4\xc4\xc4   "
"\xda\xc4\xc4\xc4\xc4\xc4\xc4\n\e[24C\e[0m\xda\xc4\xc4   "
"\xda\xc4\xc4 \xda\xc4\xc4   \xda\xc4\xc4 \xda\xc4\xc4   "
"\xda\xc4\xc4\n\e[24C\e[1;30m\xda\xc4\xc4\xc4\xc4\xc4\xc4\xc4  "
"\xda\xc4\xc4\xc4\xc4\xc4\xc4\xc4  "
"\xda\xc4\xc4\n\e[24C\e[34m\xda\xc4\xc4\e[7C\xda\xc4\xc4\e[7C\xda\xc4\xc4   "
"\xda\xc4\xc4\n\e[24C\e[0;34m\xda\xc4\xc4\e[7C\xda\xc4\xc4\e[8C\xda\xc4\xc4\xc4\xc4\xc4\xc4\n\n";

static const vcp CONSOLE_BG = VC_BLACK;

void drawLogo()
{
	MemMapFile img(ppc_img, sizeof ppc_img);
	Gif g;
	g.load(img);
	gDisplay->fillRGB(0, 0, gDisplay->mClientChar.width,
		gDisplay->mClientChar.height, MK_RGB(0xff, 0xff, 0xff));
	g.draw(gDisplay, (gDisplay->mClientChar.width-g.mWidth)/2, (gDisplay->mClientChar.height >= 600) ? (150-g.mHeight)/2 : 0);
	gDisplay->setAnsiColor(VCP(VC_BLUE, CONSOLE_BG));
	gDisplay->fillAllVT(VCP(VC_BLUE, CONSOLE_BG), ' ');
//	gDisplay->print(textlogo);
	gDisplay->setAnsiColor(VCP(VC_LIGHT(VC_BLUE), VC_TRANSPARENT));
	gDisplay->print("\e[H"APPNAME" "APPVERSION" "COPYRIGHT"\n\n");
	gDisplay->displayShow();
}

void tests()
{
/*	while (true) {
		DisplayEvent ev;
		if (gDisplay->getEvent(ev)) {
			if (ev.type == evMouse) {
				gDisplay->printf("%x, %x  ", ev.mouseEvent.relx, ev.mouseEvent.rely);
				gDisplay->printf("%x\n", ev.mouseEvent.button1);
			} else {
				gDisplay->printf("%x %d\n", ev.keyEvent.keycode, ev.keyEvent.keycode);
			}
			gDisplay->displayShow();
		}
	}*/
}

#include "io/prom/forth.h"
void testforth()
{

#if 0
		ForthVM vm;
		gCPU.msr = MSR_IR | MSR_DR | MSR_FP;
//		LocalFile in("test/test.f2", IOAM_READ, FOM_EXISTS);
//		vm.interprete(in, in);
		do {
			try {
				MemoryFile in(0);
				char buf[1024];
				fgets(buf, sizeof buf, stdin);
				in.write(buf, strlen(buf));
				in.seek(0);
				vm.interprete(in, in);
			} catch (ForthException *fe) {
				String res;
				fe->reason(res);
				ht_printf("exception: %y\n", &res);
			}
		} while (1);

#endif
}

/*
 *
 */
void usage() 
{
	ht_printf("usage: ppc configfile\n");
	exit(1);
}

int main(int argc, char *argv[])
{
/*	SYS_FILE *a = sys_fopen("test\\c.img", SYS_OPEN_CREATE | SYS_OPEN_WRITE);
	sys_fseek(a, 516096ULL*6200-1);
	byte b= 1;
	sys_fwrite(a, &b, 1);
	sys_fclose(a);*/

	if (argc != 2) {
		usage();
	}
	setvbuf(stdout, 0, _IONBF, 0);
	
	sys_gui_init();
	
 	if (sizeof(uint8) != 1) {
		ht_printf("sizeof(uint8) == %d != 1\n", sizeof(uint8)); exit(-1);
	}
	if (sizeof(uint16) != 2) {
		ht_printf("sizeof(uint16) == %d != 2\n", sizeof(uint16)); exit(-1);
	}
	if (sizeof(uint32) != 4) {
		ht_printf("sizeof(uint32) == %d != 4\n", sizeof(uint32)); exit(-1);
	}
	if (sizeof(uint64) != 8) {
		ht_printf("sizeof(uint64) == %d != 8\n", sizeof(uint64)); exit(-1);
	}

#if defined(WIN32) || defined(__WIN32__)
#else
	strncpy(gAppFilename, argv[0], sizeof gAppFilename);
#endif

	if (!initAtom()) return 3;
	if (!initData()) return 4;
	if (!initSystem()) return 5;
	try {
		gConfig = new ConfigParser();
		gConfig->acceptConfigEntryIntDef("ppc_start_resolution", GRAPHIC_MODE_800_600_15);
		gConfig->acceptConfigEntryIntDef("memory_size", 128*1024*1024);
		gConfig->acceptConfigEntryIntDef("page_table_pa", 0x00300000);
		gConfig->acceptConfigEntryIntDef("redraw_interval_msec", 200);
		prom_init_config();
		io_init_config();
		cpu_init_config();
		debugger_init_config();

		try {
			LocalFile *config;
			config = new LocalFile(argv[1]);
			gConfig->load(*config);
			delete config;
		} catch (Exception *e) {
			String res;
			e->reason(res);
			ht_printf("%s: %y\n", argv[1], &res);
			usage();
			exit(1);
		}

		ht_printf("This program is free software; you can redistribute it and/or modify\n"
			"it under the terms of the GNU General Public License version 2 as published by\n"
			"the Free Software Foundation.\n"
			"\n"
			"This program is distributed in the hope that it will be useful,\n"
			"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
			"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
			"GNU General Public License for more details.\n"
			"\n"
			"You should have received a copy of the GNU General Public License\n"
			"along with this program; if not, write to the Free Software\n"
			"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA\n\n");


		if (gConfig->getConfigInt("memory_size") < 64*1024*1024) {
			ht_printf("%s: 'memory_size' must be >= 64MB.", argv[1]);
			exit(1);
		}
		int msec = gConfig->getConfigInt("redraw_interval_msec");
		if (msec < 1 || msec > 999) {
			ht_printf("%s: 'redraw_interval_msec' must be between 1 and 999.", argv[1]);
			exit(1);
		}
		
		int gm = gConfig->getConfigInt("ppc_start_resolution");
		if (gm >= MAX_GRAPHIC_MODES) {
			ht_printf("%s: invalid '%s'\n", argv[1], "ppc_start_resolution");
			exit(1);
		}
		
		/*
		 *	begin hardware init
		 */
		
		if (!ppc_init_physical_memory(gConfig->getConfigInt("memory_size"))) {
			ht_printf("cannot initialize memory.\n");
			exit(1);
		}
		if (!cpu_init()) {
			ht_printf("cpu_init failed! Out of memory?\n");
			exit(1);
		}
		io_init();
		ppc_dec_init();

		/* 
		 *	now we have all hardware initialized
		 *	go for the gui.. 
		 *	(the menu can only be inited when the hardware has parsed
		 *	its config files.)
		 */
		gDisplay = allocSystemDisplay(APPNAME" "APPVERSION, gGraphicModes[gm]);

		MemMapFile font(ppc_font, sizeof ppc_font);
		// FIXME: ..
		if (gDisplay->mClientChar.height >= 600) {
			int width = (gDisplay->mClientChar.width-40)/8;
			int height = (gDisplay->mClientChar.height-170)/15;
			if (!gDisplay->openVT(width, height, (gDisplay->mClientChar.width-width*8)/2, 150, font)) {
				ht_printf("Can't open virtual terminal.\n");
				exit(1);
			}
		} else {
			if (!gDisplay->openVT(77, 25, 12, 100, font)) {
				ht_printf("Can't open virtual terminal.\n");
				exit(1);
			}
		}

		initMenu();
		drawLogo();
		
		// now gDisplay->printf works
		gDisplay->printf("CPU: PVR=%08x\n", gCPU.pvr);
		gDisplay->printf("%d MiB RAM\n", gMemorySize / (1024*1024));

		tests();

		// initialize initial paging (for prom)
		uint32 PAGE_TABLE_ADDR = gConfig->getConfigInt("page_table_pa");
		gDisplay->printf("initializing initial page table at %08x\n", PAGE_TABLE_ADDR);

 		// 256 Kbytes Pagetable, 2^15 Pages, 2^12 PTEGs
		if (!ppc_mmu_set_sdr1(PAGE_TABLE_ADDR+0x03, false)) {
			ht_printf("internal error setting sdr1.\n");
			return 1;
		}
		// initialize pagetable
		byte *pt;
		if (ppc_direct_physical_memory_handle(PAGE_TABLE_ADDR+256*1024, pt)
		||  ppc_direct_physical_memory_handle(PAGE_TABLE_ADDR, pt)) {
			ht_printf("cannot access page table.\n");
			return 1;
		}
		
		
		// clear pagetable
		memset(pt, 0, 256*1024);

		// init prom
		prom_init();
		
		// lock pagetable
		for (uint32 pa = PAGE_TABLE_ADDR; pa < (PAGE_TABLE_ADDR + 256*1024); pa += 4096) {
			if (!prom_claim_page(pa)) {
				ht_printf("cannot claim page table memory.\n");
				exit(1);
			}
		}
		gDisplay->displayShow();

		testforth();

		if (!prom_load_boot_file()) {
			return 1;
		}

		// this was your last chance to visit the config..
		delete gConfig;

		// use BAT for framebuffer
		gCPU.dbatu[0] = IO_GCARD_FRAMEBUFFER_EA|(7<<2)|0x3;
		gCPU.dbat_bl17[0] = ~(BATU_BL(gCPU.dbatu[0])<<17);
		gCPU.dbatl[0] = IO_GCARD_FRAMEBUFFER_PA_START;

		gDisplay->print("now starting client...");
		gDisplay->setAnsiColor(VCP(VC_WHITE, CONSOLE_BG));
		gDisplay->startRedrawThread(msec);
		ppc_run();
		delete gDisplay;		
	} catch (std::exception *e) {
		ht_printf("main() caught exception: %s\n", e->what());
		return 1;
	} catch (std::exception &e) {
		ht_printf("main() caught exception: %s\n", e.what());
		return 1;
	} catch (Exception *e) {
		String res;
		e->reason(res);
		ht_printf("main() caught exception: %y\n", &res);
		return 1;
	}
	doneData();
	doneAtom();
	return 0;
}

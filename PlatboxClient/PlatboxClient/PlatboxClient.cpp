// Written by Enrique Nissim (IOActive) 06/2018

#include "PlatboxClient.h"

/*
extern "C" {
	void _run_attempt();
}*/


FUZZER_OBJECT *g_fuzzer;


void InitializeDeltaFuzz() {
	CreateFuzzerObject(&g_fuzzer, 0, TRUE, FALSE);
}


HANDLE open_platbox_device() {
	HANDLE h = CreateFileW(
		L"\\\\.\\PlatboxDev",
		FILE_READ_ACCESS | FILE_WRITE_ACCESS,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	if (h == NULL || (int)h == -1) {
		printf("Error: %08x\n", GetLastError());
		printf("Check Platbox driver is loaded!\n");
		exit(-1);
	}

	return h;
}

void close_platbox_device(HANDLE h) {
	CloseHandle(h);
}

void print_smi_call(SW_SMI_CALL *smi) {
	printf("SwSmiNumber: %02x\n", smi->SwSmiNumber);
	printf("SwSmiData: %02x\n", smi->SwSmiData);
	printf("rax: %p\n", smi->rax);
	printf("rbx: %p\n", smi->rbx);
	printf("rcx: %p\n", smi->rcx);
	printf("rdx: %p\n", smi->rdx);
	printf("rsi: %p\n", smi->rsi);
	printf("rdi: %p\n", smi->rdi);
	Sleep(500);
}

void fuzz_smi_number(HANDLE h, int sw_smi_num) {
	DWORD bytesReturned;
	SW_SMI_CALL smi_call = { 0 };
	NTSTATUS status;
	UINT32 smram_base = 0;
	UINT32 smram_size = 0;
	printf("--> Fuzzing SW SMI Handlers 00-FF IO B2h\n");
	get_smrr(h, &smram_base, &smram_size);
	printf("-> SMRAM base: %08x\n", smram_base);
	printf("-> SMRAM size: %08x\n", smram_size);
	printf("-> TSEG: %08x\n", 0);
	printf("-> SEED: %08x", g_fuzzer->Seed);
	
	printf("\n--> SwSmiNumber: %02x\n", sw_smi_num);
	int j = 0;
	for (; ; ) {			

		g_fuzzer->fuzz_blob(g_fuzzer, (UCHAR *)&smi_call, sizeof(SW_SMI_CALL));

		UINT32 offset = g_fuzzer->get_random(g_fuzzer) % smram_size;
		UINT32 ptr = smram_base + offset;

		if (g_fuzzer->get_random(g_fuzzer) & 1)
			smi_call.rax = ptr;
		if (g_fuzzer->get_random(g_fuzzer) & 1)
			smi_call.rbx = ptr;
		if (g_fuzzer->get_random(g_fuzzer) & 1)
			smi_call.rcx = ptr;
		if (g_fuzzer->get_random(g_fuzzer) & 1)
			smi_call.rdx = ptr;
		if (g_fuzzer->get_random(g_fuzzer) & 1)
			smi_call.rsi = ptr;
		if (g_fuzzer->get_random(g_fuzzer) & 1) {
			smi_call.rdi = ptr;
		}
		smi_call.SwSmiNumber = sw_smi_num;
		smi_call.SwSmiData = g_fuzzer->get_random(g_fuzzer) % 256;
		//printf("\r--> iteration: %d", j);
		print_smi_call(&smi_call);
		status = DeviceIoControl(h, IOCTL_ISSUE_SW_SMI, &smi_call, sizeof(SW_SMI_CALL), NULL, 0, &bytesReturned, NULL);
		j++;
	}	
}


void fuzz_all_smi(HANDLE h) {
	DWORD bytesReturned;
	SW_SMI_CALL smi_call = { 0 };
	NTSTATUS status;
	UINT32 smram_base = 0;
	UINT32 smram_size = 0;
	printf("--> Fuzzing SW SMI Handlers 00-FF IO B2h\n");
	get_smrr(h, &smram_base, &smram_size);
	printf("-> SMRAM base: %08x\n", smram_base);
	printf("-> SMRAM size: %08x\n", smram_size);
	printf("-> TSEG: %08x\n", 0);
	printf("-> SEED: %08x", g_fuzzer->Seed);	
	for (int i = 0x00; i < 0x100; i++) {
		

		printf("\n--> SwSmiNumber: %02x\n", i);
		for (int j = 0; j < 10000; j++) {
			
			g_fuzzer->fuzz_blob(g_fuzzer, (UCHAR *)&smi_call, sizeof(SW_SMI_CALL));

			UINT32 offset = g_fuzzer->get_random(g_fuzzer) % smram_size;
			UINT32 ptr = smram_base + offset;

			if (g_fuzzer->get_random(g_fuzzer) & 1)
				smi_call.rax = ptr;
			if (g_fuzzer->get_random(g_fuzzer) & 1) {
				if (g_fuzzer->get_random(g_fuzzer) & 1)
					smi_call.rbx = ptr;
				else
					smi_call.rbx = 0x24483139;
			}
				
			if (g_fuzzer->get_random(g_fuzzer) & 1)
				smi_call.rcx = ptr;
			if (g_fuzzer->get_random(g_fuzzer) & 1)
				smi_call.rdx = ptr;
			if (g_fuzzer->get_random(g_fuzzer) & 1)
				smi_call.rsi = ptr;
			if (g_fuzzer->get_random(g_fuzzer) & 1) {
				smi_call.rdi = ptr;
			}
			smi_call.SwSmiNumber = i;
			smi_call.SwSmiData = g_fuzzer->get_random(g_fuzzer) % 256;
			printf("\r--> iteration: %d", j);
			status = DeviceIoControl(h, IOCTL_ISSUE_SW_SMI, &smi_call, sizeof(SW_SMI_CALL), NULL, 0, &bytesReturned, NULL);			
		}
	}
}


char _ret_shellcode[] = {
	//"\xc3" // ret
	"\x53\x56\x57\xFA\x48\x31\xC9\x48\xC7\xC2\x0F\x00\x00\x00\x4D\x31\xC0\x48\x31\xC0\xB0\x01\xC1\xE0\x1F\xC1\xE1\x10\xC1\xE2\x0B\x41\xC1\xE0\x08\x09\xC8\x09\xD0\x44\x09\xC0\x48\x89\xC3\x48\x8B\x7C\x24\x40\xCC\x48\x31\xC9\x48\xF7\xD1\x48\xFF\xC1\x48\x89\xC8\xC1\xE0\x02\x09\xD8\x66\xBA\xF8\x0C\xEF\x48\x31\xC0\x66\xBA\xFC\x0C\xED\x89\x04\x8F\x80\xF9\x40\x75\xE0\xFB\x5F\x5E\x5B\xC3"
};

void execute_shellcode(HANDLE h) {
	DWORD bytesReturned = 0;
	NTSTATUS status;

	char *shellcode = &_ret_shellcode[0];
	int size = 0x200;

	status = DeviceIoControl(h, IOCTL_EXECUTE_SHELLCODE, shellcode, size, NULL, 0, &bytesReturned, NULL);
}


void get_chipset_information(HANDLE h) {
	WORD PlatformVID, PlatformDID = 0;
	WORD PCHVID, PCHDID = 0;
	WORD GMCH = 0;
	BYTE SMRAMC;
	DWORD64 PXPEPBAR, PCIEXBAR, MCHBAR, DMIBAR, MSEGBASE, MSEGLIMIT = 0;
	DWORD64 REMAPBASE, REMAPLIMIT, TOM, TOUUD = 0;
	DWORD DEVEN, PAVPC, DPR, BDSM, BGSM, TSEGMB, TOLUD = 0;
	DWORD cpu_info[4];
	DWORD smram_base, smram_limit;
	DWORD ACPI_BAR, PWRM_BAR, ACPI_CTL = 0;
	DWORD SPIBar0Mmio, BiosSPIBDE, BiosCtl = 0;
	DWORD BFPREG, HSFS_CTL, BIOS_FADDR, BIOS_DLOCK = 0;
	DWORD FREG0, FREG1, FREG2, FREG3, FREG4, FREG5 = 0;
	DWORD FPR0, FPR1, FPR2, FPR3, FPR4 = 0;

	get_chipset_id(h, &PlatformVID, &PlatformVID, &PCHVID, &PCHDID);
	read_hostbridge_pxpepbar(h, &PXPEPBAR);
	read_hostbridge_mchbar(h, &MCHBAR);
	read_hostbridge_gmch(h, &GMCH);
	read_hostbridge_deven(h, &DEVEN);
	read_hostbridge_pavpc(h, &PAVPC);
	read_hostbridge_dpr(h, &DPR);
	read_hostbridge_pciexbar(h, &PCIEXBAR);
	read_hostbridge_dmibar(h, &DMIBAR);
	read_hostbridge_meseg_base(h, &MSEGBASE);
	read_hostbridge_meseg_limit(h, &MSEGLIMIT);
	read_hostbridge_smramc(h, &SMRAMC);
	read_hostbridge_remap_base(h, &REMAPBASE);
	read_hostbridge_remap_limit(h, &REMAPLIMIT);
	read_hostbridge_tom(h, &TOM);
	read_hostbridge_touud(h, &TOUUD);
	read_hostbridge_bdsm(h, &BDSM);
	read_hostbridge_bgsm(h, &BGSM);
	read_hostbridge_tseg(h, &TSEGMB);
	read_hostbridge_tolud(h, &TOLUD);
	get_smrr(h, (UINT32 *) &smram_base, (UINT32 *)&smram_limit);
	__cpuid((int *)&cpu_info, 1);
	BYTE SteppingID = cpu_info[0] & 0xF;
	BYTE ModelNumber = (cpu_info[0]>>4) & 0xF;
	BYTE FamilyCode = (cpu_info[0] >> 8) & 0xF;
	BYTE ProcessorType = (cpu_info[0] >> 12) & 0x3;
	BYTE ExtendedModel = (cpu_info[0] >> 0x10) & 0xF;
	BYTE ExtendedFamily = (cpu_info[0] >> 0x14) & 0xFF;

	read_pmc_acpi_base_address(h, &ACPI_BAR);
	read_pmc_pm_base_address(h, &PWRM_BAR);
	read_pmc_acpi_control(h, &ACPI_CTL);

	read_spi_interface_bar0_mmio(h, &SPIBar0Mmio);
	read_spi_interface_bios_decode_enable(h, &BiosSPIBDE);
	read_spi_interface_bios_control(h, &BiosCtl);

	read_spi_bar_bios_bfpreg(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &BFPREG);
	read_spi_bar_hsfs_ctl(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &HSFS_CTL);
	read_spi_bar_faddr(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &BIOS_FADDR);
	read_spi_bar_dlock(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &BIOS_DLOCK);
	read_spi_bar_freg0(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &FREG0);
	read_spi_bar_freg1(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &FREG1);
	read_spi_bar_freg2(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &FREG2);
	read_spi_bar_freg3(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &FREG3);
	read_spi_bar_freg4(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &FREG4);
	read_spi_bar_freg5(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &FREG5);
	read_spi_bar_fpr0(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &FPR0);
	read_spi_bar_fpr1(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &FPR1);
	read_spi_bar_fpr2(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &FPR2);
	read_spi_bar_fpr3(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &FPR3);
	read_spi_bar_fpr4(h, (DWORD64)((PCI_BAR *)&SPIBar0Mmio)->BaseAddress << 4, &FPR4);

	printf("-----------------------------------------\n");
	printf("ProcessorType: %02x\n", ProcessorType);
	printf("SteppingID: %02x\n", SteppingID);
	printf("ModelNumber: %02x\n", ModelNumber);
	printf("ExtendedModel: %02x\n", ExtendedModel);
	printf("FamilyCode: %02x\n", FamilyCode);
	printf("ExtendedFamily: %02x\n", ExtendedFamily);
	printf("-----------------------------------------\n");
	printf("+] Processor\n");
	printf(" -> VID: %04x\n", PlatformVID);
	printf(" -> DID: %04x\n", PlatformDID);
	printf("+] PCH\n");
	printf(" -> VID: %04x\n", PCHVID);
	printf(" -> DID: %04x\n", PCHDID);
	printf("-----------------------------------------\n");
	printf(" -> SMRR SMRAM Base: %08x - Limit: %08x\n", smram_base, smram_limit);
	check_memory_lock_msr(h);	
	check_ia32_msr_feature_control(h);
	check_smm_msr_feature_control(h);
	printf("-----------------------------------------\n");
	printf("+] HostBridge / DRAM Controller Registers\n");
	print_pxpepbar(&PXPEPBAR);
	print_mchbar(&MCHBAR);
	print_gmch(&GMCH);
	print_deven(&DEVEN);
	print_pavpc(&PAVPC);
	print_dpr(&DPR);
	print_pciexbar(&PCIEXBAR);
	print_dmibar(&DMIBAR);
	print_mesegbase(&MSEGBASE);
	print_meseglimit(&MSEGLIMIT);
	print_smramc(&SMRAMC);
	print_remapbase(&REMAPBASE);
	print_remaplimit(&REMAPLIMIT);	
	print_tom(&TOM);
	print_touud(&TOUUD);
	print_bdsm(&BDSM);
	print_bgsm(&BGSM);
	print_tsegmb(&TSEGMB);
	print_tolud(&TOLUD);
	printf("-----------------------------------------\n");	
	printf("+] Power Management / ACPI\n");
	print_pmc_acpi_base_address(&ACPI_BAR);
	print_pmc_pm_base_address(&PWRM_BAR);
	print_pmc_acpi_control(&ACPI_CTL);
	printf("-----------------------------------------\n");
	printf("+] BIOS - SPI Interface\n");
	print_spi_interface_bar0_mmio(&SPIBar0Mmio);
	print_spi_interface_bios_decode_enable(&BiosSPIBDE);
	print_spi_interface_bios_control(&BiosCtl);
	printf("-----------------------------------------\n");
	printf("+] BIOS - SPI BAR\n");
	
	//print_spi_bar_faddr(&BIOS_FADDR);
	//print_spi_bar_dlock(&BIOS_DLOCK);
	print_spi_bar_freg0(&FREG0);
	print_spi_bar_freg1(&FREG1);
	print_spi_bar_freg2(&FREG2);
	print_spi_bar_freg3(&FREG3);
	print_spi_bar_freg4(&FREG4);
	print_spi_bar_freg5(&FREG5);

	print_spi_bar_hsfs_ctl(&HSFS_CTL);
	print_spi_bar_bios_bfpreg(&BFPREG);

	print_spi_bar_fpr0(&FPR0);
	print_spi_bar_fpr1(&FPR1);
	print_spi_bar_fpr2(&FPR2);
	print_spi_bar_fpr3(&FPR3);
	print_spi_bar_fpr4(&FPR4);
}

void show_help() {
	printf("Commands:\n");
	printf("pci r 0:2:0 -> reads the configuration space\n");
	printf("pci rb 0:2:0 0 -> reads a byte from offset 0\n");
	printf("pci rw 0:2:0 4 -> reads a word from offset 4\n");
	printf("pci rd 0:2:0 4 -> reads a dword from offset 4\n");
	printf("pci wb 0:2:0 4 0x90 -> writes a byte to offset 4\n");
	printf("pci ww 0:2:0 4 0x9090 -> writes a word to offset 4\n");
	printf("pci wd 0:2:0 0xC 0x90909090 -> writes a dword to offset C\n");
	printf("physmem r 0x00000000 0x100 -> reads 0x100 bytes from physical address 0\n");
	printf("physmem r 0x00000000 0x100 file.bin -> reads 0x100 bytes from physical address 0 into file.bin\n");
	printf("rdmsr 0x1F2 -> reads MSR 0x1F2\n");
	printf("uefivars -> dump UEFI variables -> requires admin privileges\n");
	printf("fuzzsmi 0x44 -> fuzz SW SMI number 0x44\n");
	printf("fuzzsmi -> fuzz ALL SW SMI\n");
	printf("chipset -> throws information about several platform configuration registers\n");
	printf("--\n");
}

int main()
{	
	InitializeDeltaFuzz();
	HANDLE hDevice = open_platbox_device();

	char command_line[512];
	char **args;
	BOOL exit = FALSE;
	while (exit != TRUE) {
		printf(">>> ");
		fflush(stdout);
		get_user_input(command_line, sizeof(command_line));
		if (strlen(command_line) == 0) {
			continue;
		}
		args = parse_arguments(command_line, ' ');
		int argc = (int) args[0];
		argc--;
		char **argv = &args[1];

		if (!strcmp(argv[0], "pci")) {
			parse_handle_pci_operation(hDevice, argc, argv);			
		}
		else if (!strcmp(argv[0], "physmem")) {
			parse_handle_physmem_operation(hDevice, argc, argv);
		} 
		else if (!strcmp(argv[0], "rdmsr")) {
			read_msr(hDevice, argc, argv);
		}
		else if (!strcmp(argv[0], "wrmsr")) {
			write_msr(hDevice, argc, argv);
		}
		else if (!strcmp(argv[0], "get_smrr")) {
			get_smrr(hDevice, NULL, NULL);
		}		
		else if (!strcmp(argv[0], "fuzzsmi")) {			
			if (argc > 1) { 
				fuzz_smi_number(hDevice, strtoul(argv[1], NULL, 16));
			}
			else { // fuzzsmi
				fuzz_all_smi(hDevice);
			}			
		} 
		else if (!strcmp(argv[0], "chipset")) {
			get_chipset_information(hDevice);
		}
		else if (!strcmp(argv[0], "uefivars")) {
			list_uefi_variables();
		}
		else if (!strcmp(argv[0], "acpidump")) {
			EnumerateACPITables();
		}
		else if (!strcmp(argv[0], "?") || !strcmp(argv[0], "help")) {
			show_help();
		}
		else if (!strcmp(argv[0], "exit")) {
			exit = TRUE;
		}
		free(args);
	}


	close_platbox_device(hDevice);
	
    return 0;
}


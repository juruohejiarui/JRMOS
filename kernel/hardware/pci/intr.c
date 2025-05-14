#include <hardware/pci.h>
#include <hal/hardware/pci.h>
#include <lib/bit.h>
#include <screen/screen.h>

void hw_pci_initIntr(intr_Desc *desc, void (*handler)(u64), u64 param, char *name) {
	desc->handler = handler;
	desc->param = param;
	desc->name = name;
	desc->ctrl = &hw_pci_intrCtrl;
}

int hw_pci_setMsix(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrNum) {
    return hal_hw_pci_setMsix(cap, cfg, desc, intrNum);
}

int hw_pci_setMsi(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrNum) {
    hw_pci_MsiCap_setVecNum(cap, intrNum);
    return hal_hw_pci_setMsi(cap, desc, intrNum);
}

void hw_pci_disableIntx(hw_pci_Cfg *cfg) {
    bit_set1_16(&cfg->command, 10);
}
int hw_pci_allocMsi(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrNum) {
	if (hw_pci_MsiCap_vecNum(cap) < intrNum) {
		printk(RED, BLACK, "hw: pci: alloc too much interrupt for msi, vecNum=%d, require %d interrupt\n", hw_pci_MsiCap_vecNum(cap), intrNum);
		return res_FAIL;
	}
	if (intr_alloc(desc, intrNum) == res_FAIL) {
		printk(RED, BLACK, "hw: pci: alloc msi interrupt failed\n");
		return res_FAIL;
	}
	if (hw_pci_setMsi(cap, desc, intrNum) == res_FAIL) {
		printk(RED, BLACK, "hw: pci: set msi interrupt failed\n");
		intr_free(desc, intrNum);
		return res_FAIL;
	}
	return res_SUCC;
}

int hw_pci_allocMsix(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrNum) {
	if (hw_pci_MsixCap_vecNum(cap) < intrNum) {
		printk(RED, BLACK, "hw: pci: alloc too much interrupt for msix, vecNum=%d, require %d interrupt\n", hw_pci_MsixCap_vecNum(cap), intrNum);
		return res_FAIL;
	}
	for (int i = 0; i < intrNum; i++) {
		if (intr_alloc(&desc[i], 1) == res_FAIL) {
			printk(RED, BLACK, "hw: pci: alloc msix interrupt failed\n");
			for (int j = 0; j < i; j++) {
				intr_free(&desc[j], 1);
			}
			return res_FAIL;
		}
	}
	if (hw_pci_setMsix(cap, cfg, desc, intrNum) == res_FAIL) {
		printk(RED, BLACK, "hw: pci: set msix interrupt failed\n");
		for (int i = 0; i < intrNum; i++) {
			intr_free(&desc[i], 1);
		}
		return res_FAIL;
	}
	return res_SUCC;
}

int hw_pci_enableMsi(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrIdx) {
    if (desc->ctrl == NULL && desc->ctrl->enable != NULL) {
        if (desc->ctrl->enable(desc) == res_FAIL) return res_FAIL;
    }
    /// @todo add lock
    bit_set0_32(hw_pci_MsiCap_msk(cap), intrIdx);
    return res_SUCC;
}

int hw_pci_enableMsix(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrIdx) {
    if (desc->ctrl != NULL && desc->ctrl->enable != NULL) {
        if (desc->ctrl->enable(desc) == res_FAIL) return res_FAIL;
    }
    hw_pci_MsixTbl *tbl = hw_pci_getMsixTbl(cap, cfg);
    bit_set0_32(&tbl[intrIdx].vecCtrl, 0);
    return res_SUCC;
}

int hw_pci_disableMsi(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrIdx) {
    if (desc->ctrl == NULL && desc->ctrl->enable != NULL) {
        if (desc->ctrl->disable(desc) == res_FAIL) return res_FAIL;
    }
    bit_set1_32(hw_pci_MsiCap_msk(cap), intrIdx);
    return res_SUCC;
}

int hw_pci_disableMsix(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrIdx) {
    if (desc->ctrl == NULL && desc->ctrl->enable != NULL) {
        if (desc->ctrl->disable(desc) == res_FAIL) return res_FAIL;
    }

    hw_pci_MsixTbl *tbl = hw_pci_getMsixTbl(cap, cfg);
    bit_set1_32(&tbl[intrIdx].vecCtrl, 0);
    return res_SUCC;
}

int hw_pci_enableMsiAll(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrNum) {
    for (int i = 0; i < intrNum; i++) 
        if ((*hw_pci_MsiCap_msk(cap) & (1u << intrNum)) && hw_pci_enableMsi(cap, &desc[i], i) == res_FAIL) return res_FAIL;
    bit_set1_16(hw_pci_MsiCap_msgCtrl(cap), 0);
    return res_SUCC;
}

int hw_pci_enableMsixAll(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrNum) {
    hw_pci_MsixTbl *tbl = hw_pci_getMsixTbl(cap, cfg);
    for (int i = 0; i < intrNum; i++) 
        if ((tbl[i].vecCtrl & 1) && hw_pci_enableMsix(cap, cfg, &desc[i], i) == res_FAIL) return res_FAIL;
    bit_set1_16(&cap->msgCtrl, 15);
    bit_set0_16(&cap->msgCtrl, 14);
    return res_SUCC;
}

int hw_pci_disableMsiAll(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrNum) {
    bit_set0_16(hw_pci_MsiCap_msgCtrl(cap), 0);
    for (int i = 0; i < intrNum; i++) 
        if (!(*hw_pci_MsiCap_msk(cap) & (1u << intrNum)) && hw_pci_disableMsi(cap, &desc[i], i) == res_FAIL) return res_FAIL;
    return res_SUCC;
}

int hw_pci_disableMsixAll(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrNum) {
    bit_set1_16(&cap->msgCtrl, 14);
    hw_pci_MsixTbl *tbl = hw_pci_getMsixTbl(cap, cfg);
    for (int i = 0; i < intrNum; i++) 
        if (!(tbl->vecCtrl & 1) && hw_pci_disableMsix(cap, cfg, &desc[i], i) == res_FAIL) return res_FAIL;
    return res_SUCC;
}

int hw_pci_freeMsi(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrNum) {
    if (hw_pci_disableMsiAll(cap, desc, intrNum) == res_FAIL) return res_FAIL;
    if (intr_free(desc, intrNum) == res_FAIL) return res_FAIL;
    return res_SUCC;
}

int hw_pci_freeMsix(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrNum) {
    if (hw_pci_disableMsixAll(cap, cfg, desc, intrNum) == res_FAIL) return res_FAIL;
    if (intr_free(desc, intrNum) == res_FAIL) return res_FAIL;
    return res_SUCC;
}
# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux Systems Network Architecture layer.
#

ifneq ($(KERNELRELEASE),)
# kbuild part of makefile

obj-m := sna.o

ccflags-y := -I$(src)/../include
sna-y := af_sna.o sna_ebcdic.o sna_nof.o sna_cs.o sna_asm.o sna_cosm.o \
	    sna_cpic.o sna_ds.o sna_pc.o sna_rss.o sna_ss.o sna_tdm.o \
	    sna_dlc.o sna_trs.o sna_rm.o sna_sm.o sna_ms.o \
	    sna_hsr.o sna_tc.o sna_dfc.o sna_attach.o \
	    sna_ps_main.o sna_ps_conv.o sna_ps_copr.o sna_ps_mc.o \
	    sna_ps_sync.o sna_vector.o

sna-$(CONFIG_SYSCTL) += sysctl_net_sna.o

else
# normal makefile

KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

endif

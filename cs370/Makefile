# Makefile for building and running wrappers


# Path to tools dir
TOOL_DIR="./"

# Path to wrapper sources
WRAPPER_DIR="./wrappers"

# Path to qcoe file for dev vm
KDEV_VM="kerneldev-vm.qcow2"

# Path to kernel image
KDEV_VM_BOOT="../arch/x86_64/boot/bzImage"

# Path to kernel source tree
KDEV_DIR="../"

# Number of threads to run
KDEV_OPT_NTHREADS=8

# EXTRAVERSION to pass during kernel build
KDEV_OPT_VERSION=".19-FOX-NELSON"


.PHONY: clean build_kernel deploy_wrappers

#Build the kernel from source
build_kernel:
	@cd $(KDEV_DIR); make EXTRAVERSION='$(KDEV_OPT_VERSION)' -j$(KDEV_OPT_NTHREADS)

# Need recipe to build kernel image
$(KDEV_VM_BOOT): build_kernel

run_vm: $(KDEV_VM_BOOT)
	@kvm -curses -kernel $(KDEV_VM_BOOT) -append 'root=/dev/hda1 ro' -drive file=$(KDEV_VM) --redir tcp:2222::22    
   
build_wrappers:
	@cd $(WRAPPER_DIR); make build_all;

deploy_wrappers: build_wrappers run_vm
     
	
clean:
	@cd $(KDEV_DIR); make clean
	@cd $(WRAPPER_DIR); make clean

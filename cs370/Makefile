# Makefile for building and running wrappers


# Path to tools dir
TOOL_DIR="./"

# Path to wrapper sources
WRAPPER_DIR="./wrappers"

# Path to qcoe file for dev vm
KDEV_VM="kerneldev-vm.qcow2"

# Path to kernel image
KDEV_VM_BOOT="../arch/x86_64/boot/bzImage"

# Port on host system to direct SSH to
KDEV_VM_REDIR_PORT="2222"

# Directory on vm host that holds the bins
KDEV_VM_BIN_DIR="/home/user/"

# Path to kernel source tree
KDEV_DIR="../"

# Number of threads to run
KDEV_OPT_NTHREADS=8

# EXTRAVERSION to pass during kernel build
KDEV_OPT_VERSION=".19-FOX-NELSON"


# Location of SSH identity file for use with VM
KDEV_SSH_IDRSA="./tools/ssh/kdev-vm_user-id_rsa"

# SSH port to use
KDEV_SSH_PORT=$(KDEV_VM_REDIR_PORT)

# SSH user name
KDEV_SSH_USER="user"

# SSH host
KDEV_SSH_HOST="localhost"

# Number of seconds to sleep on deploy of wrapper bins
# this gives the vm enough time to initialize network
# and services
KDEV_SSH_SLEEP_SEC="0"

#### RECIPES ####

.PHONY: clean build_kernel deploy_wrappers shut_vm

#Build the kernel from source
build_kernel:
	@cd $(KDEV_DIR); make EXTRAVERSION='$(KDEV_OPT_VERSION)' -j$(KDEV_OPT_NTHREADS)

# Need recipe to build kernel image
$(KDEV_VM_BOOT): build_kernel

# Run VM
run_vm: $(KDEV_VM_BOOT)
	@kvm -curses -kernel $(KDEV_VM_BOOT) -append 'root=/dev/hda1 ro' -drive file=$(KDEV_VM) --redir tcp:$(KDEV_VM_REDIR_PORT)::22
   
# Build just the wrappers
build_wrappers:
	@cd $(WRAPPER_DIR); make build_all;

# Build wrappers and also start the vm
deploy_wrappers: build_wrappers
	@chmod 700 $(KDEV_SSH_IDRSA)
	@echo "Sleeping $(KDEV_SSH_SLEEP_SEC) secconds ..."
	@sleep $(KDEV_SSH_SLEEP_SEC)
	@echo "Cleaning up working dir on VM ..."
	@ssh -p $(KDEV_SSH_PORT) -i $(KDEV_SSH_IDRSA) $(KDEV_SSH_USER)@$(KDEV_SSH_HOST) "rm -rf $(KDEV_VM_BIN_DIR)/*bin"
	@echo "Deploying new bins on VM ... "
	@scp -P $(KDEV_SSH_PORT) -i $(KDEV_SSH_IDRSA) $(WRAPPER_DIR)/*.bin $(KDEV_SSH_USER)@$(KDEV_SSH_HOST):$(KDEV_VM_BIN_DIR)
	@echo "Logging into VM ... "
	@ssh -p $(KDEV_SSH_PORT) -i $(KDEV_SSH_IDRSA) $(KDEV_SSH_USER)@$(KDEV_SSH_HOST)

shut_vm:
	@chmod 700 $(KDEV_SSH_IDRSA)
	@echo "Logging in as Root on VM ... "
	@ssh -p $(KDEV_SSH_PORT) -i $(KDEV_SSH_IDRSA) root@$(KDEV_SSH_HOST) "halt"
	@echo "VM Shutdown in progress ... "
	
     
clean:
	@cd $(KDEV_DIR); make clean
	@cd $(WRAPPER_DIR); make clean

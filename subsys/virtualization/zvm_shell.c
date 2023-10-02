/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel/thread.h>
#include <shell/shell.h>
#include <logging/log.h>
#include <virtualization/vm_manager.h>
#include <virtualization/vm.h>

#define SHELL_HELP_ZVM "ZVM manager command. " \
    "Some subcommand you can choice as below:"  \
    "new set run update list delete"
#define SHELL_HELP_CREATE_NEW_VM "Create a new vm.\n"
#define SHELL_HELP_RUN_VM "Run vm x.\n"
#define SHELL_HELP_UPDATE_VM "Update vm x.\n"
#define SHELL_HELP_LIST_VM "List all vm info.\n"
#define SHELL_HELP_PAUSE_VM "Pause vm x.\n"
#define SHELL_HELP_DELETE_VM "Delete vm x.\n"
#define SHELL_HELP_RUN_DEFAULT_VM "Run init zephyr VM here. \n"

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

/* A spinlock for shell */
static struct k_spinlock shell_vmops_lock;

/**
 * @brief New a vm in the system.
 */
static int cmd_zvm_new(const struct shell *shell, size_t argc, char **argv)
{
    int ret = 0;
    k_spinlock_key_t key;

	key = k_spin_lock(&shell_vmops_lock);
	shell_fprintf(shell, SHELL_NORMAL, "Ready to create a new vm... \n");

    ret = zvm_new_guest(argc, argv);
    if (ret) {
        shell_fprintf(shell, SHELL_NORMAL,
            "Create vm failured, please follow the message and try again! \n");
        k_spin_unlock(&shell_vmops_lock, key);
        return ret;
    }
    k_spin_unlock(&shell_vmops_lock, key);

    return ret;
}


/**
 * @brief Run a exit vm in the system.
 */
static int cmd_zvm_run(const struct shell *shell, size_t argc, char **argv)
{
    /* Run vm code. */
    int ret = 0;
    k_spinlock_key_t key;

	key = k_spin_lock(&shell_vmops_lock);

    ret = zvm_run_guest(argc, argv);
    if (ret) {
        shell_fprintf(shell, SHELL_NORMAL,
            "Start vm failured, please follow the message and try again! \n");
        k_spin_unlock(&shell_vmops_lock, key);
        return ret;
    }

    k_spin_unlock(&shell_vmops_lock, key);

    return ret;
}

/**
 * @brief pause a vm.
 */
static int cmd_zvm_pause(const struct shell *shell, size_t argc, char **argv)
{
    int ret = 0;
    k_spinlock_key_t key;

    key = k_spin_lock(&shell_vmops_lock);
    ret = zvm_pause_guest(argc, argv);
    if (ret) {
        shell_fprintf(shell, SHELL_NORMAL,
            "Pause vm failured, please follow the message and try again! \n");
        k_spin_unlock(&shell_vmops_lock, key);
        return ret;
    }

    k_spin_unlock(&shell_vmops_lock, key);

    return ret;
}


/**
 * @brief delete a vm.
 */
static int cmd_zvm_delete(const struct shell *shell, size_t argc, char **argv)
{
    int ret = 0;
    k_spinlock_key_t key;

    key = k_spin_lock(&shell_vmops_lock);

    /* Delete vm code. */
    ret = zvm_delete_guest(argc, argv);
    if (ret) {
        shell_fprintf(shell, SHELL_NORMAL,
            "Delete vm failured, please follow the message and try again! \n");
        k_spin_unlock(&shell_vmops_lock, key);
        return ret;
    }
    k_spin_unlock(&shell_vmops_lock, key);

    return ret;
}


/**
 * @brief List a vm's info.
 */
static int cmd_zvm_info(const struct shell *shell, size_t argc, char **argv)
{
    int ret = 0;
    k_spinlock_key_t key;

    key = k_spin_lock(&shell_vmops_lock);

    /* Delete vm code. */
    ret = zvm_info_guest(argc, argv);
    if (ret) {
        shell_fprintf(shell, SHELL_NORMAL,
            "List vm failured. \n There may no vm in the system! \n");
        k_spin_unlock(&shell_vmops_lock, key);
        return ret;
    }
    k_spin_unlock(&shell_vmops_lock, key);

    return 0;
}

/**
 * @brief update a vm in the system.
 */
static int cmd_zvm_update(const struct shell *shell, size_t argc, char **argv)
{
    /* Update vm code. */
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_fprintf(shell, SHELL_NORMAL,
            "Update vm is not support now, Please try other command. \n");
    return 0;
}


/* Add subcommand for Root0 command zvm. */
SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_zvm,
    SHELL_CMD(new, NULL, SHELL_HELP_CREATE_NEW_VM, cmd_zvm_new),
    SHELL_CMD(run, NULL, SHELL_HELP_RUN_VM, cmd_zvm_run),
    SHELL_CMD(pause, NULL, SHELL_HELP_PAUSE_VM, cmd_zvm_pause),
    SHELL_CMD(delete, NULL, SHELL_HELP_DELETE_VM, cmd_zvm_delete),
    SHELL_CMD(info, NULL, SHELL_HELP_LIST_VM, cmd_zvm_info) ,
    SHELL_CMD(update, NULL, SHELL_HELP_UPDATE_VM, cmd_zvm_update),
    SHELL_SUBCMD_SET_END
);

/* Add command for hypervisor. */
SHELL_CMD_REGISTER(zvm, &m_sub_zvm, SHELL_HELP_ZVM, NULL);

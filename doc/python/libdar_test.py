import libdar
import os, sys

# mandatory first action is to initialize libdar
# by calling libdar.get_version()
u = libdar.get_version()
print("using libdar version {}.{}.{}".format(u[0],u[1],u[2]))
# libdar.get_version() can be called at will

# now defining a very minimalist class to let libdar
# interact with the user directly. One could make use
# of graphical popup window here if a Graphical User Interface
# was used:
class myui(libdar.user_interaction):
    def __init__(self):
        libdar.user_interaction.__init__(self)
        # it is mandatory to initialize the parent
        # class: libdar.user_interaction

    def inherited_message(self, msg):
        print("LIBDAR MESSAGE:{0}".format(msg))

    def inherited_pause(self, msg):
        while True:
            res = input("LIBDAR QUESTION:{0} y/n ".format(msg))
            if res == "y":
                return True
            else:
                if res == "n":
                    return False
                else:
                    print("answer 'y' or 'n'")

    def inherited_get_string(self, msg, echo):
        return input(msg)

    def inherited_get_secu_string(self, msg, echo):
        return input(msg)


def i2str(infinint):
    return libdar.deci(infinint).human()

def display_stats(stats):
    print("---- stats result ---")
    print("treated entries  = {}".format(stats.get_treated_str()))
    print("hard link entries = {}".format(stats.get_hard_links_str()))
    print("%skipped entries = {}".format(stats.get_skipped_str()))
    print("inode only entries  = {}".format(stats.get_inode_only_str()))
    print("ignored entries  = {}".format(stats.get_ignored_str()))
    print("too old entries  = {}".format(stats.get_tooold_str()))
    print("errored entries  = {}".format(stats.get_errored_str()))
    print("deleted entries  = {}".format(stats.get_deleted_str()))
    print("EA      entries  = {}".format(stats.get_ea_treated_str()))
    print("FSA     entries  = {}".format(stats.get_fsa_treated_str()))
    print("hard link entries = {}".format(stats.get_hard_links_str()))
    print("wasted byte amount = {}".format(stats.get_byte_amount_str()))
    print("total enries = {}".format(i2str(stats.total())))
    print("---------------------")

def display_entree_stats(stats, ui):
    print("--- archive content stats ---")
    stats.listing(ui)
    print("-----------------------------")

def list_dir(archive, chem = "", indent = ""):
    content = archive.get_children_in_table(chem, True)
    for ent in content:
        ligne = indent
        if ent.is_eod():
            continue

        if ent.is_hard_linked():
            ligne += "*"
        else:
            ligne += " "

        if ent.is_dir():
            ligne += "d"
        else:
            if ent.is_file():
                ligne += "f"
            else:
                 if ent.is_symlink():
                     ligne += "l"
                 else:
                     if ent.is_char_device():
                         ligne += "c"
                     else:
                         if ent.is_block_device():
                             ligne += "b"
                         else:
                             if ent.is_unix_socket():
                                 ligne += "s"
                             else:
                                 if ent.is_named_pipe():
                                     ligne += "p"
                                 else:
                                     if ent.is_door_inode():
                                         ligne += "D"
                                     else:
                                         if ent.is_removed_entry():
                                             ligne += "Removed entry which was of of type {}".format(ent.get_removed_type())
                                         else:
                                             ligne += "WHAT THIS????"
                                             continue

        ligne += ent.get_perm() + " " + ent.get_name() + " "
        ligne += ent.get_uid(True) + "/" + ent.get_gid(True) + " "
        ligne += ent.get_last_modif()
        print(ligne)
        if ent.is_dir():
            if chem != "":
                nchem = (libdar.path(chem) + ent.get_name()).display()
            else:
                nchem = ent.get_name()
            nindent = indent + "   "
            list_dir(archive, nchem, nindent)

ui = myui()
sauv_path = libdar.path(".")
arch_name = "arch1"
ext = "dar"



def f1():
    opt = libdar.archive_options_create()
    opt.set_info_details(True)
    opt.set_display_treated(True, False)
    opt.set_display_finished(True)
    fsroot = libdar.path("/etc")
    print("creating the archive")
    libdar.archive(ui,
                   fsroot,
                   sauv_path,
                   arch_name,
                   ext,
                   opt)

def f2():
    opt = libdar.archive_options_create()
    opt.set_info_details(True)
    opt.set_display_treated(False, False)
    opt.set_display_finished(True)
    fsroot = libdar.path("/etc")
    stats = libdar.statistics()

    print("creating the archive")
    libdar.archive(ui,
                   fsroot,
                   sauv_path,
                   arch_name,
                   ext,
                   opt,
                   stats)
    display_stats(stats)


def f3():
    opt = libdar.archive_options_read()
    opt.set_info_details(True)
    arch1 = libdar.archive(ui,
                           sauv_path,
                           arch_name,
                           ext,
                           opt);
    list_dir(arch1)
    stats = arch1.get_stats()
    display_entree_stats(stats, ui)


def f4():
    opt = libdar.archive_options_read()
    arch1 = libdar.archive(ui,
                           sauv_path,
                           arch_name,
                           ext,
                           opt);

    opt = libdar.archive_options_test()

    # defining which file to test bases on filename (set_selection)
    mask_file1 = libdar.simple_mask("*.*", True)
    mask_file2 = libdar.regular_mask(".*\.pub$", True)
    mask_filenames = libdar.ou_mask() # doing the logical OR between what we will add to it:
    mask_filenames.add_mask(mask_file1)
    mask_filenames.add_mask(mask_file2)
    opt.set_selection(mask_filenames)

    # reducing the testing in subdirectories

    tree = libdar.et_mask() # doing the loical AND betwen what we will add to it:
    tree.add_mask(libdar.not_mask(tree1))
    tree.add_mask(libdar.not_mask(tree2))
    opt.set_subtree(tree)
    
    opt.set_info_details(True)
    opt.set_display_skipped(True)
    
    arch1.op_test(opt)

def f5():
    opt = libdar.archive_options_read()
    arch1 = libdar.archive(ui,
                           sauv_path,
                           arch_name,
                           ext,
                           opt);
    
    tree1 = libdar.simple_path_mask("/etc/ssh", False)
    tree2 = libdar.simple_path_mask("/etc/grub.d", False)
    tree = libdar.ou_mask()
    tree.add_mask(tree1)
    tree.add_mask(tree2)
    
    opt = libdar.archive_options_diff()
    opt.set_subtree(tree)
    opt.set_info_details(True)
    opt.set_display_treated(True, False)
    opt.set_ea_mask(libdar.bool_mask(True))
    opt.set_furtive_read_mode(False)

    arch1.op_diff(libdar.path("/etc"),
                  opt)

    rest = libdar.path("./Restore")
    try:
        os.rmdir(rest.display())
    except:
        pass
    os.mkdir(rest.display())

    opt = libdar.archive_options_extract()
    over_policy = libdar.crit_constant_action(libdar.over_action_data.data_preserve,
                                           libdar.over_action_ea.EA_preserve)
    opt.set_overwriting_rules(over_policy)
    fsa_scope = set()
    fsa_scope.add(libdar.fsa_family.fsaf_hfs_plus)
    fsa_scope.add(libdar.fsa_family.fsaf_linux_extX)
    opt.set_fsa_scope(fsa_scope)
    stats = libdar.statistics()
    arch1.op_extract(rest, opt, stats)
    display_stats(stats)


def f6():
    opt = libdar.archive_options_read()
    passwd ="joe@the.shmoe"
    secu_pass = libdar.secu_string(passwd, len(passwd))
    entrepot = libdar.entrepot_libcurl(ui,
                                       libdar.mycurl_protocol.proto_ftp,
                                       "anonymous",
                                       secu_pass,
                                       "ftp.dm3c.org",
                                       "",
                                       False,
                                       "",
                                       "",
                                       "",
                                       5)
    print(entrepot.get_url())
    opt.set_entrepot(entrepot)
    opt.set_info_details(True)

    arch2 = libdar.archive(ui,
                           libdar.path("/dar.linux.free.fr/"),
                           "exemple",
                           "dar",
                           opt)

    opt2 = libdar.archive_options_test()
    opt2.set_display_treated(True, False)
    arch2.op_test(opt2)
    

# After a build: copy each firmware to the project root and check that it fits the update slot.
#
# The update page can only accept an image up to (flash up to the filesystem at 0x100000) minus the
# size of the firmware that is running, rounded to 4 KB sectors, minus one sector. Going over that
# limit is what produces "Update error: ERROR[4]: Not Enough Space" on the device.
Import("env")
import os, shutil

NAMES = {"installer": "SmallTV-Installer.bin", "smalltv": "SmallTV-Modern.bin"}
STOCK_SIZE = 505200          # original firmware (FW-Smalltv-Ultra-V9_0_50.bin)
MARGIN = 8192                # keep a safety margin below the limit

def limit_for(running_size):
    used = (running_size + 4095) & ~4095
    return ((0x100000 - used) - 0x1000) & ~0xFFF

def copy_bin(source, target, env):
    proj = env.subst("$PROJECT_DIR")
    name = NAMES.get(env["PIOENV"], env["PIOENV"] + ".bin")
    dst = os.path.join(proj, name)
    shutil.copyfile(str(target[0]), dst)
    size = os.path.getsize(dst)
    if name == "SmallTV-Installer.bin":
        limit, via = limit_for(STOCK_SIZE), "the original firmware's update page"
    else:
        inst = os.path.join(proj, "SmallTV-Installer.bin")
        if not os.path.exists(inst):
            print("\n>>> Wrote %s (%d bytes)" % (dst, size)); return
        limit, via = limit_for(os.path.getsize(inst)), "the installer's upload page"
    print("\n>>> Wrote %s: %d bytes. Limit through %s: %d bytes (%.0f KB to spare)" % (name, size, via, limit, (limit - size) / 1024.0))
    if size > limit - MARGIN:
        print(">>> ERROR: %s is too big and would fail on the device with 'ERROR[4] Not Enough Space'." % name)
        os.remove(dst)
        env.Exit(1)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_bin)

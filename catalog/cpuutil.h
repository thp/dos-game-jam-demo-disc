#pragma once

extern short get_flags(void);
#pragma aux get_flags = \
        "pushf" \
        "pop ax" \
    value [ax];

extern short check_flags_write_one(void);
#pragma aux check_flags_write_one = \
        "pushf" \
        "mov ax, 0xf000" \
        "push ax" \
        "popf" \
        "pushf" \
        "pop ax" \
        "popf" \
    value [ax];

enum CPUType {
    CPU_UNKNOWN = 0,
    CPU_8086,
    CPU_286,
    CPU_386,
};

static const char *
CPU_TYPES[] = {
    "Unknown",
    "8086",
    "286",
    "386+",
};

static enum CPUType
detect_cpu_type(void)
{
    short flags = (get_flags() & 0xf000);
    short flags_one = (check_flags_write_one() & 0xf000);

    // https://stackoverflow.com/a/47721890
    // http://www.rcollins.org/ddj/Sep96/Sep96.html

    if (flags == 0xf000) {
        // The 8086/88 will always set these bits, regardless of what values are popped into them
        return CPU_8086;
    } else if (flags == 0 && flags_one == 0) {
        // The 286 treats these bits differently. In real mode, these bits are always cleared by the 286
        return CPU_286;
    } else {
        // have at least a 386 CPU
        return CPU_386;
    }

    return CPU_UNKNOWN;
}

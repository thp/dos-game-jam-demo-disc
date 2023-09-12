; chain a DOS interrupt with restoring of DS:DX to
; original values passed into the chain function
; 2023-09-12 Thomas Perl <m@thp.io>

_TEXT   segment word public 'CODE'
_TEXT   ends


_TEXT   segment

_restore_ds_dx proc far
    pop dx
    pop ds
    iret ; return from ISR
_restore_ds_dx endp

_chain_intr_dsdx     proc far
        public  "C",_chain_intr_dsdx
        ; never return to the caller
        ; doesn't have return address on the stack

        mov     sp,bp                   ; reset SP to point to saved registers

        ; incoming variables:
        ; ax = offset
        ; dx = segment
        ; bx = ds to restore
        ; cx = dx to restore

        xchg ax,bx

        ; incoming variables:
        ; bx = offset
        ; dx = segment
        ; ax = ds to restore
        ; cx = dx to restore

        ; stack layout before (see notes.txt):
        ; bp +  0 = (dummy fs) (free for overwriting immediately)
        ; bp +  2 = (dummy gs) (free for overwriting immediately)
        ; bp +  4 = saved es
        ; bp +  6 = saved ds
        ; bp +  8 = saved di
        ; bp + 10 = saved si (can be overwritten after restore)
        ; bp + 12 = saved bp (moved)
        ; bp + 14 = saved sp (not restored)
        ; bp + 16 = saved bx (moved)
        ; bp + 18 = saved dx (can be overwritten after restore)
        ; bp + 20 = saved cx (swapped for restore)
        ; bp + 22 = saved ax (swapped for restore)
        ; bp + 24 = offset of ISR CALLER
        ; bp + 26 = segment of ISR CALLER
        ; bp + 28 = flags to restore for ISR CALLER

        xchg    cx,20[bp]               ; restore cx, & put in "dx to restore"
        xchg    ax,22[bp]               ; restore ax, & put in "ds to restore"

        mov si,10[bp] ; restore si immediately, so we can overwrite it
        mov 10[bp],bx ; store offset of ISR

        mov bx,12[bp] ; move saved bp
        mov 0[bp],bx

        mov 12[bp],dx ; store segment of ISR

        mov bx,16[bp] ; move saved bx
        mov 2[bp],bx

        mov dx,18[bp] ; restore dx immediately, so we can overwrite it

        mov bx,offset _restore_ds_dx ; load offset
        mov 14[bp],bx

        mov bx,seg _restore_ds_dx ; load segment
        mov 16[bp],bx

        mov bx,28[bp] ; load flags
        mov 18[bp],bx

        ; required stack layout:              __
        ; bp +  0 = saved bp OK                 |
        ; bp +  2 = saved bx OK                 |
        ; bp +  4 = saved es -- STAYS THE SAME  |-- Restored using "pop"
        ; bp +  6 = saved ds -- STAYS THE SAME  |
        ; bp +  8 = saved di -- STAYS THE SAME _|
        ; bp + 10 = offset of ISR to call OK  \____ Called using "ret"
        ; bp + 12 = segment of ISR to call OK /           ____
        ; bp + 14 = offset of _restore_ds_dx -- CALCULATED    |
        ; bp + 16 = segment of _restore_ds_dx -- CALCULATED   |-- Used up by "iret" - return to _restore_ds_dx
        ; bp + 18 = flags for _restore_ds_dx -- COPY FROM 28 _|
        ; bp + 20 = dx to restore OK \___ Restored by _restore_ds_dx using "pop"
        ; bp + 22 = ds to restore OK /                     ____________
        ; bp + 24 = offset of ISR CALLER -- STAYS THE SAME             |
        ; bp + 26 = segment of ISR CALLER -- STAYS THE SAME            |-- Used up by "iret" in _restore_ds_dx,
        ; bp + 28 = flags to restore for ISR CALLER -- STAYS THE SAME _|   finally return to original ISR caller

        mov     bx,28[bp]               ; restore flags
        and     bx,0FCFFh               ; except for IF and TF
        push    bx                      ; :
        popf                            ; :

        ; restore saved registers
        pop bp
        pop bx
        pop es
        pop ds
        pop di

        ; consume the offset + segment of the ISR to call
        ret

        ; the ISR will eventually "iret" to _restore_ds_dx, and this will
        ; restore the original "dx" and "ds" values from the stack and then
        ; do its own "iret" to return to the original ISR caller
_chain_intr_dsdx     endp
_TEXT ends
end

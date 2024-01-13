; Proof of concept: tool to change Compaq Portable III video to use CGA
; No checks are performed.

; Build:
; tasm setcga
; tlink setcga /t

   .MODEL tiny
   .CODE
   
ORG 0100h

start: 
   push    cs
   pop     ds

   mov     ah, 9                   
   mov     dx, OFFSET HelloMessage 
   int     21h                    

   mov     dx, 13C6h 
   in  	   al, dx
   and     al, 0F8h       ; Often AL == 08h
   out     dx, al
   
   push    ds
   mov     ax, 40h        ; BIOS Data Area segment
   mov     ds, ax         ; https://stanislavs.org/helppc/bios_data_area.html
   and     byte ptr ds:10h, 0CFh ; 
   or      byte ptr ds:10h, 20h  ; Set bits 4-5 to 10b, color 80x25
   pop     ds
   
   mov     ax, 3          ; Set video mode to 80x25/16 colors (CGA)
   int     10h            ; http://www.ctyme.com/intr/rb-0069.htm#Table10

   
   mov     ax, 4C00h      ; DOS terminate program function with 00 exit code.
   int     21h          

HelloMessage DB 'Changing Compaq Portable III mode to CGA',13,10,'$'   
   
END start 

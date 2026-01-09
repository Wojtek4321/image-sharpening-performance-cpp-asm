;Opis algorytmu:
;   Wyostrzanie obrazu metodą unsharp masking
;   dst = src + amount * (src - blur)
;   Operacje wykonywane są wektorowo na 8 pikselach jednocześnie.
;
;Wojciech Pędziwiatr
;Semestr zimowy 2025/2026


PUBLIC sharpen_avx
.code
sharpen_avx PROC


    cvtsd2ss xmm1, xmm0   ; amount w xmm0 (float)
    vbroadcastss ymm1, xmm1 ; amount w ymm1
    
    ; Konwertuj amount na 16-bit fixed point (amount * 256)
    mov eax, 43800000h          ; 256.0f
    vmovd xmm2, eax
    vmulss xmm2, xmm1, xmm2
    vcvtss2si eax, xmm2
    vmovd xmm1, eax
    vpbroadcastw ymm1, xmm1     ; amount w fixed point
    
    xor r10, r10 ; szybsze niz mov r10 , 0 licznik pikseli
    
loop_sharpen:
    cmp r10, r9 
    jae end_proc ; koniec pętli jeśli przetworzono wszystkie piksele
    
    vmovdqu ymm2, ymmword ptr [rcx + r10*4]  ; src
    vmovdqu ymm3, ymmword ptr [rdx + r10*4]  ; blur
    
    ; Oblicz różnicę z nasyceniem
    vpsubusb ymm4, ymm2, ymm3
    
    ; Rozszerz do 16-bit i przemnóż
    vpunpcklbw ymm5, ymm4, ymm4
    vpsrlw ymm5, ymm5, 8
    vpmullw ymm5, ymm5, ymm1
    vpsrlw ymm5, ymm5, 8
    
    vpunpckhbw ymm6, ymm4, ymm4
    vpsrlw ymm6, ymm6, 8
    vpmullw ymm6, ymm6, ymm1
    vpsrlw ymm6, ymm6, 8
    
    ; Pack i dodaj z nasyceniem
    vpackuswb ymm5, ymm5, ymm6
    vpaddusb ymm2, ymm2, ymm5
    
    vmovdqu ymmword ptr [r8 + r10*4], ymm2
    
    add r10, 8
    jmp loop_sharpen
    
end_proc:
    vzeroupper
    ret
sharpen_avx ENDP
END
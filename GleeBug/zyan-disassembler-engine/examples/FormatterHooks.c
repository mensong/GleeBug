/***************************************************************************************************

  Zyan Disassembler Library (Zydis)

  Original Author : Florian Bernd

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.

***************************************************************************************************/

/**
 * @file
 * @brief   Demonstrates the hooking functionality of the @c ZydisInstructionFormatter class.
 * 
 * This example demonstrates the hooking functionality of the @c ZydisInstructionFormatter class by 
 * rewriting the mnemonics of (V)CMPPS and (V)CMPPD to their corresponding alias-forms (based on  
 * the condition encoded in the immediate operand).
 */

#include <inttypes.h>
#include <Zydis/Zydis.h>
#include "FormatHelper.h"
#include <stdlib.h>
#include <time.h>

/* ============================================================================================== */
/* Static data                                                                                    */
/* ============================================================================================== */

/**
 * @brief   Static array with the condition-code strings.
 */
static const char* conditionCodeStrings[0x20] =
{
    /*00*/ "eq",
    /*01*/ "lt", 
    /*02*/ "le", 
    /*03*/ "unord", 
    /*04*/ "neq", 
    /*05*/ "nlt", 
    /*06*/ "nle", 
    /*07*/ "ord", 
    /*08*/ "eq_uq", 
    /*09*/ "nge", 
    /*0A*/ "ngt", 
    /*0B*/ "false", 
    /*0C*/ "oq", 
    /*0D*/ "ge", 
    /*0E*/ "gt", 
    /*0F*/ "true", 
    /*10*/ "eq_os", 
    /*11*/ "lt_oq", 
    /*12*/ "le_oq",
    /*13*/ "unord_s", 
    /*14*/ "neq_us", 
    /*15*/ "nlt_uq", 
    /*16*/ "nle_uq", 
    /*17*/ "ord_s", 
    /*18*/ "eq_us", 
    /*19*/ "nge_uq", 
    /*1A*/ "ngt_uq", 
    /*1B*/ "false_os", 
    /*1C*/ "neq_os", 
    /*1D*/ "ge_oq", 
    /*1E*/ "gt_oq", 
    /*1F*/ "true_us"
};

/* ============================================================================================== */
/* Hook callbacks                                                                                 */
/* ============================================================================================== */

ZydisFormatterFormatFunc defaultPrintMnemonic;

static ZydisStatus ZydisFormatterPrintMnemonic(ZydisInstructionFormatter* formatter, 
    char** buffer, size_t bufferLen, ZydisInstructionInfo* info)
{
    // We use the user-data field of the instruction-info to pass data to the 
    // @c ZydisFormatterFormatOperandImm function.
    // In this case we are using a simple ordinal value, but you could pass a pointer to a 
    // complex datatype as well.
    info->userData = (void*)1;

    // Rewrite the instruction-mnemonic for the given instructions
    if ((info->operandCount == 3) && (info->operands[2].type == ZYDIS_OPERAND_TYPE_IMMEDIATE))
    {
        uint8_t conditionCode = info->operands[2].imm.value.ubyte;
        if (conditionCode < 0x08)
        {
            switch (info->mnemonic)
            {
            case ZYDIS_MNEMONIC_CMPPS:
                return ZydisStringBufferAppendFormat(buffer, bufferLen, 
                    ZYDIS_STRBUF_APPEND_MODE_DEFAULT, "cmp%sps", 
                    conditionCodeStrings[conditionCode]);
            case ZYDIS_MNEMONIC_CMPPD:
                return ZydisStringBufferAppendFormat(buffer, bufferLen, 
                    ZYDIS_STRBUF_APPEND_MODE_DEFAULT, "cmp%spd", 
                    conditionCodeStrings[conditionCode]);
            default:
                break;
            }
        }
    }
    if ((info->operandCount == 4) && (info->operands[3].type == ZYDIS_OPERAND_TYPE_IMMEDIATE))
    {    
        uint8_t conditionCode = info->operands[3].imm.value.ubyte;
        if (conditionCode < 0x20)
        {
            switch (info->mnemonic)
            {
            case ZYDIS_MNEMONIC_VCMPPS:    
                return ZydisStringBufferAppendFormat(buffer, bufferLen, 
                    ZYDIS_STRBUF_APPEND_MODE_DEFAULT, "vcmp%sps", 
                    conditionCodeStrings[conditionCode]);
            case ZYDIS_MNEMONIC_VCMPPD:
                return ZydisStringBufferAppendFormat(buffer, bufferLen, 
                    ZYDIS_STRBUF_APPEND_MODE_DEFAULT, "vcmp%spd", 
                    conditionCodeStrings[conditionCode]);
            default:
                break;
            }
        }   
    }

    // We did not rewrite the instruction-mnemonic. Signal the @c ZydisFormatterFormatOperandImm 
    // function not to omit the operand
    info->userData = (void*)0;

    // Default mnemonic printing
    return defaultPrintMnemonic(formatter, buffer, bufferLen, info); 
}

/* ---------------------------------------------------------------------------------------------- */

ZydisFormatterFormatOperandFunc defaultFormatOperandImm;

static ZydisStatus ZydisFormatterFormatOperandImm(ZydisInstructionFormatter* formatter,
    char** buffer, size_t bufferLen, ZydisInstructionInfo* info, ZydisOperandInfo* operand)
{
    // The @c ZydisFormatterFormatMnemonic sinals us to omit the immediate (condition-code) 
    // operand, because it got replaced by the alias-mnemonic
    if ((uintptr_t)info->userData == 1)
    {
        // The formatter will automatically omit the operand, if the buffer remains unchanged 
        // after the callback returns
        return ZYDIS_STATUS_SUCCESS;    
    }

    // Default immediate formatting
    return defaultFormatOperandImm(formatter, buffer, bufferLen, info, operand);
}

/* ---------------------------------------------------------------------------------------------- */

/* ============================================================================================== */
/* Helper functions                                                                               */
/* ============================================================================================== */

void disassembleBuffer(uint8_t* data, size_t length, ZydisBool installHooks)
{
    ZydisInstructionFormatter formatter;
    ZydisFormatterInitInstructionFormatterEx(&formatter, ZYDIS_FORMATTER_STYLE_INTEL,
        ZYDIS_FMTFLAG_FORCE_SEGMENTS | ZYDIS_FMTFLAG_FORCE_OPERANDSIZE,
        ZYDIS_FORMATTER_ADDR_ABSOLUTE, ZYDIS_FORMATTER_DISP_DEFAULT, ZYDIS_FORMATTER_IMM_DEFAULT);

    if (installHooks)
    {
        defaultPrintMnemonic = &ZydisFormatterPrintMnemonic;
        ZydisFormatterSetHook(&formatter, ZYDIS_FORMATTER_HOOK_PRINT_MNEMONIC, 
            (const void**)&defaultPrintMnemonic);
        defaultFormatOperandImm = &ZydisFormatterFormatOperandImm;
        ZydisFormatterSetHook(&formatter, ZYDIS_FORMATTER_HOOK_FORMAT_OPERAND_IMM, 
            (const void**)&defaultFormatOperandImm);
    }

    uint64_t instructionPointer = 0x007FFFFFFF400000;

    ZydisInstructionInfo info;
    char buffer[256];
    while (ZYDIS_SUCCESS(
        ZydisDecode(ZYDIS_OPERATING_MODE_64BIT, data, length, instructionPointer, &info)))
    {
        data += info.length;
        length -= info.length;
        instructionPointer += info.length;
        printf("%016" PRIX64 "  ", info.instrAddress);
        ZydisFormatterFormatInstruction(&formatter, &info, &buffer[0], sizeof(buffer));  
        printf(" %s\n", &buffer[0]);
    }    
}

/* ============================================================================================== */
/* Entry point                                                                                    */
/* ============================================================================================== */

int main()
{

    uint8_t data[] = 
    {
        // cmpps xmm1, xmm4, 0x03
        0x0F, 0xC2, 0xCC, 0x03, 

        // vcmpord_spd xmm1, xmm2, xmm3
        0xC5, 0xE9, 0xC2, 0xCB, 0x17,

        // vcmpps k2 {k7}, zmm2, dword ptr ds:[rax + rbx*4 + 0x100] {1to16}, 0x0F
        0x62, 0xF1, 0x6C, 0x5F, 0xC2, 0x54, 0x98, 0x40, 0x0F
    };

    disassembleBuffer(&data[0], sizeof(data), ZYDIS_FALSE);
    puts("");
    disassembleBuffer(&data[0], sizeof(data), ZYDIS_TRUE);

    getchar();
    return 0;
}

/* ============================================================================================== */

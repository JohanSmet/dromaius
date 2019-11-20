// test/test_filt_6502_asm.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"

#include "filt_6502_asm.h"
#include "cpu_6502_opcodes.h"
#include "stb/stb_ds.h"

MunitResult test_format(const MunitParameter params[], void* user_data_or_fixture) {

	static uint8_t TEST_BINARY[] = {
		OP_6502_LDA_IMM,  0x10,			// lda #$10 (immediate)
		OP_6502_ORA_ZP,   0x0a,			// ora $0a (zero page)
		OP_6502_ASL_ZPX,  0x12,			// asl $12,x (zero page, x indexed)
		OP_6502_STX_ZPY,  0x61,			// stx $61,y (zero page, y indexed)
		OP_6502_JMP_ABS,  0x01, 0x08,	// jmp $0801 (absolute)
		OP_6502_STA_ABSX, 0x03, 0x60,	// sta $6003,x (absolute, x indexed)
		OP_6502_STA_ABSY, 0x03, 0x60,	// sta $6003,y (absolute, y indexed)
		OP_6502_JMP_IND,  0xfc, 0xff,	// jmp ($fffc)
		OP_6502_EOR_INDX, 0x71,			// eor ($71,x) 
		OP_6502_ADC_INDY, 0x72,			// adc ($72),y
		OP_6502_BCS,      0x15,			// bcs $15 
		OP_6502_ROR_ACC,				// ror (accumulator)
		OP_6502_BRK						// brk (implied)
	};

	int index = 0;	
	char *output = NULL;
	
	// immediate operand
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 2);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "8000: lda #$10");
	arrsetlen(output, 0);

	// zero page 
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 4);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "8002: ora $0a");
	arrsetlen(output, 0);

	// zero page, x
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 6);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "8004: asl $12,x");
	arrsetlen(output, 0);

	// zero page, y
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 8);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "8006: stx $61,y");
	arrsetlen(output, 0);

	// absolute
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 11);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "8008: jmp $0801");
	arrsetlen(output, 0);
	
	// absolute, x
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 14);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "800b: sta $6003,x");
	arrsetlen(output, 0);

	// absolute, y
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 17);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "800e: sta $6003,y");
	arrsetlen(output, 0);

	// indirect
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 20);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "8011: jmp ($fffc)");
	arrsetlen(output, 0);

	// X-indexed, indirect
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 22);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "8014: eor ($71,x)");
	arrsetlen(output, 0);

	// indirect, y-indexed
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 24);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "8016: adc ($72),y");
	arrsetlen(output, 0);

	// relative
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 26);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "8018: bcs $15");
	arrsetlen(output, 0);

	// accumulator
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 27);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "801a: ror");
	arrsetlen(output, 0);
	
	// implied addressing
	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, ==, 28);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "801b: brk");
	arrsetlen(output, 0);

    return MUNIT_OK;
}

MunitResult test_incomplete(const MunitParameter params[], void* user_data_or_fixture) {

	static uint8_t TEST_BINARY[] = {
		OP_6502_STA_ABS, 0x03,			// STA with absolute addressing should have 2 address bytes
	};

	int index = 0;
	char *output = NULL;

	index += filt_6502_asm_line(TEST_BINARY, sizeof(TEST_BINARY), index, 0x8000, &output);
	munit_assert_int(index, >, 2);
	munit_assert_not_null(output);
	munit_assert_string_equal(output, "8000: sta (opcode incomplete)");
	
	return MUNIT_OK;
}

MunitTest filt_6502_asm_tests[] = {
    { "/printf", test_format, NULL, NULL,  MUNIT_TEST_OPTION_NONE, NULL },
    { "/incomplete", test_incomplete, NULL, NULL,  MUNIT_TEST_OPTION_NONE, NULL },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

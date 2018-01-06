#include <boost/test/unit_test.hpp>

#include "computer.hpp"
#include "test_fixture.hpp"

using namespace IBM650;

BOOST_AUTO_TEST_CASE(off)
{
    Computer computer;
    BOOST_CHECK(!computer.is_on());
    BOOST_CHECK(!computer.is_blower_on());
    BOOST_CHECK(!computer.is_ready());
}

BOOST_AUTO_TEST_CASE(turn_on)
{
    Computer computer;
    computer.power_on();
    BOOST_CHECK(computer.is_on());
    BOOST_CHECK(computer.is_blower_on());
    BOOST_CHECK(!computer.is_ready());
    computer.step(179);
    BOOST_CHECK(!computer.is_ready());
    computer.step(1);
    BOOST_CHECK(computer.is_ready());
}

BOOST_AUTO_TEST_CASE(turn_off)
{
    Computer_Ready_Fixture f;
    BOOST_CHECK(f.computer.is_on());
    BOOST_CHECK(f.computer.is_blower_on());
    BOOST_CHECK(f.computer.is_ready());
    f.computer.power_off();
    BOOST_CHECK(!f.computer.is_on());
    BOOST_CHECK(f.computer.is_blower_on());
    BOOST_CHECK(!f.computer.is_ready());
    f.computer.step(299);
    BOOST_CHECK(f.computer.is_blower_on());
    f.computer.step(1);
    BOOST_CHECK(!f.computer.is_blower_on());
}

BOOST_AUTO_TEST_CASE(dc_on_off)
{
    Computer computer;
    computer.power_on();
    BOOST_CHECK(computer.is_on());
    computer.step(10);
    computer.dc_on(); // Too early; no effect.
    BOOST_CHECK(!computer.is_ready());
    computer.step(10);
    computer.dc_off(); // Doesn't prevent automatic DC turn-on.
    computer.step(180);
    BOOST_CHECK(computer.is_on());
    BOOST_CHECK(computer.is_blower_on());
    BOOST_CHECK(computer.is_ready());
    computer.dc_off();
    BOOST_CHECK(computer.is_on());
    BOOST_CHECK(computer.is_blower_on());
    BOOST_CHECK(!computer.is_ready());
    computer.step(10);
    BOOST_CHECK(!computer.is_ready());
    computer.step(1800);
    computer.dc_on();
    BOOST_CHECK(computer.is_on());
    BOOST_CHECK(computer.is_blower_on());
    BOOST_CHECK(computer.is_ready());
    computer.power_off();
    computer.step(500);
    BOOST_CHECK(!computer.is_ready());
    computer.dc_on();
    BOOST_CHECK(!computer.is_ready());
}

BOOST_AUTO_TEST_CASE(master_power)
{
    Computer_Ready_Fixture f;
    f.computer.master_power_off();
    BOOST_CHECK(!f.computer.is_on());
    BOOST_CHECK(!f.computer.is_blower_on());
    BOOST_CHECK(!f.computer.is_ready());
    f.computer.power_on();
    // Can't be turned back on.
    BOOST_CHECK(!f.computer.is_on());
    BOOST_CHECK(!f.computer.is_blower_on());
    BOOST_CHECK(!f.computer.is_ready());
    f.computer.dc_on();
    BOOST_CHECK(!f.computer.is_on());
    BOOST_CHECK(!f.computer.is_blower_on());
    BOOST_CHECK(!f.computer.is_ready());
}

BOOST_AUTO_TEST_CASE(transfer)
{
    Computer_Ready_Fixture f;
    f.computer.set_control_mode(Computer::Control_Mode::manual);
    f.computer.set_address(Address({1,2,3,4}));
    BOOST_CHECK(f.computer.address_register() != Address({1,2,3,4}));
    f.computer.transfer();
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({1,2,3,4}));
    f.computer.set_control_mode(Computer::Control_Mode::address_stop);
    f.computer.set_address(Address({0,0,9,9}));
    f.computer.transfer();
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({1,2,3,4}));
    f.computer.set_control_mode(Computer::Control_Mode::run);
    f.computer.set_address(Address({0,0,9,9}));
    f.computer.transfer();
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({1,2,3,4}));
}

struct Reset_Fixture : public Computer_Ready_Fixture
{
    Reset_Fixture() {
        computer.set_address(Address({1,2,3,4}));
        computer.set_distributor(Word({3,10, 3,2,3,3, 3,4,3,5, '-'}));
        computer.set_upper(Word({10,1, 1,2,1,3, 1,4,1,5, '+'}));
        computer.set_lower(Word({2,1, 2,2,2,3, 2,4,2,5, '+'}));
        computer.set_program_register(Word({1,2, 3,4,5,6, 7,8,9,10, '+'}));
        computer.set_error();
    }
};

BOOST_AUTO_TEST_CASE(program_reset_manual)
{
    Reset_Fixture f;
    BOOST_CHECK(f.computer.program_register_validity_error());
    BOOST_CHECK(f.computer.storage_selection_error());
    BOOST_CHECK(f.computer.clocking_error());
    f.computer.set_display_mode(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({1,2, 3,4,5,6, 7,8,9,10, '_'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>({1,2}));
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({3,4,5,6}));

    f.computer.set_control_mode(Computer::Control_Mode::manual);
    f.computer.program_reset();
    BOOST_CHECK(!f.computer.program_register_validity_error());
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address());
}

BOOST_AUTO_TEST_CASE(program_reset_run)
{
    Reset_Fixture f;
    f.computer.set_control_mode(Computer::Control_Mode::run);
    f.computer.program_reset();
    BOOST_CHECK(!f.computer.program_register_validity_error());
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    f.computer.set_display_mode(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({8,0,0,0}));
}

BOOST_AUTO_TEST_CASE(program_reset_address_stop)
{
    Reset_Fixture f;
    f.computer.set_control_mode(Computer::Control_Mode::address_stop);
    f.computer.program_reset();
    BOOST_CHECK(!f.computer.program_register_validity_error());
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    f.computer.set_display_mode(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({8,0,0,0}));
}

BOOST_AUTO_TEST_CASE(accumulator_reset_manual)
{
    Reset_Fixture f;
    BOOST_CHECK(f.computer.distributor_validity_error());
    BOOST_CHECK(f.computer.accumulator_validity_error());
    BOOST_CHECK(f.computer.storage_selection_error());
    BOOST_CHECK(f.computer.clocking_error());
    f.computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({2,1, 2,2,2,3, 2,4,2,5, '+'}));
    f.computer.set_display_mode(Computer::Display_Mode::upper_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({10,1, 1,2,1,3, 1,4,1,5, '+'}));
    f.computer.set_display_mode(Computer::Display_Mode::distributor);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({3,10, 3,2,3,3, 3,4,3,5, '-'}));
    f.computer.set_display_mode(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({1,2, 3,4,5,6, 7,8,9,10, '_'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>({1,2}));
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({3,4,5,6}));

    f.computer.set_control_mode(Computer::Control_Mode::manual);
    f.computer.accumulator_reset();
    BOOST_CHECK(!f.computer.overflow());
    BOOST_CHECK(!f.computer.distributor_validity_error());
    BOOST_CHECK(!f.computer.accumulator_validity_error());
    // Program register still has a bad digit.
    BOOST_CHECK(f.computer.program_register_validity_error());
    // Address register is still out of range: 3456.
    BOOST_CHECK(f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    f.computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_display_mode(Computer::Display_Mode::upper_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_display_mode(Computer::Display_Mode::distributor);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));

    // Unchanged.
    f.computer.set_display_mode(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({1,2, 3,4,5,6, 7,8,9,10, '_'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>({1,2}));
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({3,4,5,6}));
}

BOOST_AUTO_TEST_CASE(error_reset)
{
    Reset_Fixture f;
    f.computer.error_reset();
    BOOST_CHECK(f.computer.overflow());
    BOOST_CHECK(f.computer.distributor_validity_error());
    BOOST_CHECK(f.computer.accumulator_validity_error());
    BOOST_CHECK(f.computer.program_register_validity_error());
    BOOST_CHECK(f.computer.error_sense());
    // Still has bad address.
    BOOST_CHECK(f.computer.storage_selection_error());
    f.computer.set_program_register(Word({1,2, 1,2,3,4, 2,4,6,8, '-'}));
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
}

BOOST_AUTO_TEST_CASE(error_sense_reset)
{
    Reset_Fixture f;
    BOOST_CHECK(f.computer.error_sense());
    f.computer.error_sense_reset();
    BOOST_CHECK(!f.computer.error_sense());
}

BOOST_AUTO_TEST_CASE(computer_reset_manual)
{
    Reset_Fixture f;
    f.computer.set_control_mode(Computer::Control_Mode::manual);
    f.computer.computer_reset();
    BOOST_CHECK(!f.computer.program_register_validity_error());
    BOOST_CHECK(!f.computer.storage_selection_error());
    BOOST_CHECK(!f.computer.clocking_error());
    BOOST_CHECK(!f.computer.error_sense());
    f.computer.set_display_mode(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    f.computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_display_mode(Computer::Display_Mode::upper_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_display_mode(Computer::Display_Mode::distributor);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address());
}

BOOST_AUTO_TEST_CASE(computer_reset_run)
{
    Reset_Fixture f;
    f.computer.set_control_mode(Computer::Control_Mode::run);
    f.computer.computer_reset();
    f.computer.set_display_mode(Computer::Display_Mode::program_register);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '_'}));
    f.computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_display_mode(Computer::Display_Mode::upper_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_display_mode(Computer::Display_Mode::distributor);
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({8,0,0,0}));
}

BOOST_AUTO_TEST_CASE(storage_entry)
{
    Word word({1,2, 1,2,3,4, 2,4,6,8, '-'});
    Address word_addr({0,5,1,2});
    Word zero({0,0, 0,0,0,0, 0,0,0,0, '+'});
    Address zero_addr({0,5,1,3});

    Computer_Ready_Fixture f;
    // See operator's manual p.55 "Examples of Use of the Control Console"
    f.computer.set_storage_entry(word);
    f.computer.set_address(word_addr);
    f.computer.set_control_mode(IBM650::Computer::Control_Mode::manual);
    f.computer.set_display_mode(IBM650::Computer::Display_Mode::read_in_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();
    f.computer.set_display_mode(Computer::Display_Mode::distributor);
    BOOST_CHECK_EQUAL(f.computer.display(), word);

    f.computer.set_storage_entry(zero);
    f.computer.set_address(zero_addr);
    f.computer.set_display_mode(IBM650::Computer::Display_Mode::read_in_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();
    f.computer.set_display_mode(Computer::Display_Mode::distributor);
    BOOST_CHECK_EQUAL(f.computer.display(), zero);

    f.computer.set_address(word_addr);
    f.computer.set_display_mode(Computer::Display_Mode::read_out_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();
    BOOST_CHECK_EQUAL(f.computer.display(), word);

    f.computer.set_address(zero_addr);
    f.computer.set_display_mode(Computer::Display_Mode::read_out_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();
    BOOST_CHECK_EQUAL(f.computer.display(), zero);
}

BOOST_AUTO_TEST_CASE(accumulator_entry)
{
    Word word({1,2, 1,2,3,4, 2,4,6,8, '-'});
    Address word_addr({0,5,1,2});

    Computer_Ready_Fixture f;
    f.computer.set_storage_entry(word);
    f.computer.set_address(word_addr);
    f.computer.set_control_mode(IBM650::Computer::Control_Mode::manual);
    f.computer.set_display_mode(IBM650::Computer::Display_Mode::read_in_storage);
    f.computer.program_reset();
    f.computer.transfer();
    f.computer.program_start();

    // f.computer.set_display_mode(Computer::Display_Mode::distributor);
    // BOOST_CHECK_EQUAL(f.computer.display(), word);

    // op 65 is "reset add lower".
    f.computer.set_storage_entry(Word({6,5, 0,5,1,2, 0,0,0,0, '+'}));
    f.computer.set_half_cycle_mode(Computer::Half_Cycle_Mode::half);
    f.computer.set_control_mode(Computer::Control_Mode::run);
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), word_addr);
    // The I-half-cycle is ready to be executed.
    f.computer.program_reset();
    BOOST_CHECK(!f.computer.data_address());
    BOOST_CHECK(f.computer.instruction_address());
    f.computer.program_start();

    // The 1st half-cycle loads the program register from the storage-entry switches.
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>({6,5}));
    BOOST_CHECK_EQUAL(f.computer.address_register(), word_addr);
    // The D-half-cycle is ready to be executed.
    BOOST_CHECK(f.computer.data_address());
    BOOST_CHECK(!f.computer.instruction_address());
    f.computer.program_start();
    BOOST_CHECK_EQUAL(f.computer.operation_register(), Register<2>());
    BOOST_CHECK_EQUAL(f.computer.address_register(), Address({0,0,0,0}));
    f.computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), word);
}

BOOST_AUTO_TEST_CASE(accumulator_entry_2)
{
    Word word({1,2, 2,4,6,8, 1,2,3,4, '-'});

    Computer_Ready_Fixture f;
    f.computer.set_display_mode(Computer::Display_Mode::distributor);

    // "Reset add lower" from storage entry switches.
    f.computer.set_storage_entry(Word({6,5, 8,0,0,0, 0,0,0,0, '+'}));
    f.computer.set_half_cycle_mode(Computer::Half_Cycle_Mode::half);
    f.computer.set_control_mode(Computer::Control_Mode::run);
    f.computer.program_reset();
    f.computer.program_start();

    f.computer.set_storage_entry(word);
    f.computer.program_start();

    f.computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
    BOOST_CHECK_EQUAL(f.computer.display(), word);
}

BOOST_AUTO_TEST_CASE(start_program)
{
    // Add to upper
    Word instr1({1,0, 1,1,0,0, 0,1,0,1, '+'});
    Address addr1({0,1,0,0});
    // Add to upper
    Word instr2({1,0, 1,1,0,0, 0,1,0,8, '+'});
    Address addr2({0,1,0,1});
    // Program stop
    Word instr3({0,1, 0,0,0,0, 0,0,0,0, '+'});
    Address addr3({0,1,0,8});
    // Data
    Word data({0,0, 0,0,0,1, 7,1,7,1, '+'});
    Address data_addr({1,1,0,0});

    // Assuming that the sign is not displayed with the upper accumulator.
    Word data_display({0,0, 0,0,0,1, 7,1,7,1, '_'});

    Computer_Ready_Fixture f;
    f.computer.set_drum(addr1, instr1);
    f.computer.set_drum(addr2, instr2);
    f.computer.set_drum(addr3, instr3);
    f.computer.set_drum(data_addr, data);
    // Make the program stop on "program stop".
    f.computer.set_programmed_mode(Computer::Programmed_Mode::stop);
    f.computer.set_control_mode(Computer::Control_Mode::run);
    f.computer.set_display_mode(Computer::Display_Mode::upper_accumulator);
    f.computer.set_storage_entry(Word({0,0, 0,0,0,0, 0,1,0,0, '+'}));

    f.computer.computer_reset();
    f.computer.program_start();
    BOOST_CHECK_EQUAL(f.computer.display(), Word({0,0, 0,0,0,3, 4,3,4,2, '+'}));
}

struct LD_Fixture : public Run_Fixture
{
    LD_Fixture()
        : LD({6,9, 0,1,0,0, 0,0,0,1, '+'}),
          STOP({0,1, 0,0,0,0, 0,0,0,0, '+'}),
          data({0,0, 0,1,1,2, 2,3,3,4, '-'})
        {
            computer.set_drum(Address({0,0,0,0}), LD);
            computer.set_drum(Address({0,0,0,1}), STOP);
            computer.set_drum(Address({0,1,0,0}), data);
            computer.set_storage_entry(Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
            computer.set_display_mode(Computer::Display_Mode::distributor);
        }

    Word LD;
    Word STOP;
    Word data;
};

BOOST_AUTO_TEST_CASE(run_LD)
{
    LD_Fixture f;
    f.computer.computer_reset();
    f.computer.program_start();
    BOOST_CHECK_EQUAL(f.computer.display(), f.data);
}

BOOST_AUTO_TEST_CASE(run_LD_twice)
{
    // Check that the program can be run again after reset.
    LD_Fixture f;
    f.computer.computer_reset();
    std::cerr << "start 1\n";
    f.computer.program_start();
    BOOST_CHECK_EQUAL(f.computer.display(), f.data);

    f.computer.computer_reset();
    std::cerr << "start 2\n";
    f.computer.program_start();
    BOOST_CHECK_EQUAL(f.computer.display(), f.data);
}

BOOST_AUTO_TEST_CASE(LD_timing)
{
    LD_Fixture f;
    f.computer.computer_reset();
    f.computer.program_start();
    // Drum index = 0
    // 1 to enable PR
    // 0 for address search (8000)
    // 2 to fill PR, OP, DA to ADDR
    // 0 for no-op
    // 2 to for IA to ADDR, enable PR
    // Drum index = 5
    // 45 to find inst addr 0000 on drum
    // t = 50
    // 2 to fill PR, OP, DA to ADDR
    // 1 to enable distributor
    // Drum index = 3
    // 47 to find data addr 0100 on drum.
    // t = 100
    // 1 to load distributor.
    // 2 to for IA to ADDR, enable PR
    // Drum index = 3
    // 48 to find inst addr 0001 on drum.
    // t = 151
    // 2 to fill PR, OP, DA to ADDR
    // 0 for stop
    // 2 to for IA to ADDR, enable PR
    BOOST_CHECK_EQUAL(f.computer.run_time(), 155);
    BOOST_CHECK_EQUAL(f.computer.display(), f.data);
}

struct Optimum_LD_Fixture : public Run_Fixture
{
    Optimum_LD_Fixture()
        : LD({6,9, 0,1,0,8, 0,0,6,1, '+'}),
          STOP({0,1, 0,0,0,0, 0,0,0,0, '+'}),
          data({0,0, 0,1,1,2, 2,3,3,4, '-'})
        {
            computer.set_drum(Address({0,0,0,5}), LD);
            computer.set_drum(Address({0,0,6,1}), STOP);
            computer.set_drum(Address({0,1,0,8}), data);
            computer.set_storage_entry(Word({0,0, 0,0,0,0, 0,0,0,5, '+'}));
            computer.set_display_mode(Computer::Display_Mode::distributor);
        }

    Word LD;
    Word STOP;
    Word data;
};

BOOST_AUTO_TEST_CASE(optimum_LD_timing)
{
    Optimum_LD_Fixture f;
    f.computer.computer_reset();
    f.computer.program_start();
    // Drum index = 0
    // 1 to enable PR
    // 0 for address search (8000)
    // 2 to fill PR, OP, DA to ADDR
    // 0 for no-op
    // 2 to for IA to ADDR, enable PR
    // Drum index = 5
    // 0 to find inst addr 0005 on drum
    // t = 5
    // 2 to fill PR, OP, DA to ADDR
    // 1 to enable distributor
    // Drum index = 8
    // 0 to find data addr 0108 on drum.
    // t = 8
    // 1 to load distributor.
    // 2 to for IA to ADDR, enable PR
    // Drum index = 11
    // 0 to find inst addr 0061 on drum.
    // t = 11
    // 2 to fill PR, OP, DA to ADDR
    // 0 for stop
    // 2 to for IA to ADDR, enable PR
    BOOST_CHECK_EQUAL(f.computer.run_time(), 15);
    BOOST_CHECK_EQUAL(f.computer.display(), f.data);
}

struct RAL_Fixture : public Run_Fixture
{
    RAL_Fixture()
        : RAL({6,5, 0,1,0,0, 0,0,0,1, '+'}),
          STOP({0,1, 0,0,0,0, 0,0,0,0, '+'}),
          data({0,0, 0,1,1,2, 2,3,3,4, '-'})
        {
            computer.set_drum(Address({0,0,0,0}), RAL);
            computer.set_drum(Address({0,0,0,1}), STOP);
            computer.set_drum(Address({0,1,0,0}), data);
            computer.set_storage_entry(Word({0,0, 0,0,0,0, 0,0,0,0, '+'}));
            computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
        }

    Word RAL;
    Word STOP;
    Word data;
};

BOOST_AUTO_TEST_CASE(RAL_timing)
{
    RAL_Fixture f;
    f.computer.computer_reset();
    std::cerr << "start\n";
    f.computer.program_start();
    // Drum index = 0
    // 1 to enable PR
    // 0 for address search (8000)
    // 2 to fill PR, OP, DA to ADDR
    // 0 for no-op
    // 2 to for IA to ADDR, enable PR
    // Drum index = 5
    // 45 to find inst addr 0000 on drum
    // t = 50
    // 2 to fill PR, OP, DA to ADDR
    // 1 to enable distributor
    // Drum index = 3
    // 47 to find data addr 0100 on drum.
    // t = 100
    // 1 to load distributor.
    // 1 wait for even
    // 2 fill accumulator (restart and IA to ADDR)
    // 1 remove interlock A (enable PR)
    // Drum index = 4
    // 47 to find inst addr 0001 on drum.
    // t = 151
    // 2 to fill PR, OP, DA to ADDR
    // 0 for stop
    // 2 to for IA to ADDR, enable PR
    BOOST_CHECK_EQUAL(f.computer.run_time(), 155);
    BOOST_CHECK_EQUAL(f.computer.display(), f.data);
}

struct Optimum_RAL_Fixture : public Run_Fixture
{
    Optimum_RAL_Fixture()
        : RAL({6,5, 1,1,5,8, 0,0,1,3, '+'}),
          STOP({0,1, 0,0,0,0, 0,0,0,0, '+'}),
          data({0,0, 0,1,1,2, 2,3,3,4, '-'})
        {
            computer.set_drum(Address({0,0,0,5}), RAL);
            computer.set_drum(Address({0,0,1,3}), STOP);
            computer.set_drum(Address({1,1,5,8}), data);
            computer.set_storage_entry(Word({0,0, 0,0,0,0, 0,0,0,5, '+'}));
            computer.set_display_mode(Computer::Display_Mode::lower_accumulator);
        }

    Word RAL;
    Word STOP;
    Word data;
};

BOOST_AUTO_TEST_CASE(optimum_RAL_timing)
{
    Optimum_RAL_Fixture f;
    f.computer.computer_reset();
    std::cerr << "start\n";
    f.computer.program_start();
    // Drum index = 0
    // 1 to enable PR
    // 0 for address search (8000)
    // 2 to fill PR, OP, DA to ADDR
    // 0 for no-op
    // 2 to for IA to ADDR, enable PR
    // Drum index = 5
    // 0 to find inst addr 0000 on drum
    // t = 5
    // 2 to fill PR, OP, DA to ADDR
    // 1 to enable distributor
    // Drum index = 8
    // 0 to find data addr 1158 on drum.
    // t = 8
    // 1 to load distributor.
    // 1 wait for even
    // 2 fill accumulator (restart and IA to ADDR)
    // 1 remove interlock A (enable PR)
    // Drum index = 13
    // 0 to find inst addr 0013 on drum.
    // t = 13
    // 2 to fill PR, OP, DA to ADDR
    // 0 for stop
    // 2 to for IA to ADDR, enable PR
    BOOST_CHECK_EQUAL(f.computer.run_time(), 17);
    BOOST_CHECK_EQUAL(f.computer.display(), f.data);
}

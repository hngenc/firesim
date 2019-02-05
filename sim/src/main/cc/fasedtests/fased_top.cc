//See LICENSE for license details.
#include "fasedtests_top.h"
#include "test_harness_endpoint.h"
// MIDAS-defined endpoints
#include "endpoints/fpga_memory_model.h"
#include "endpoints/synthesized_assertions.h"
#include "endpoints/synthesized_prints.h"

fasedtests_top_t::fasedtests_top_t(int argc, char** argv)
{
    std::vector<std::string> args(argv + 1, argv + argc);
    max_cycles = -1;
    profile_interval = max_cycles;

    for (auto &arg: args) {
        if (arg.find("+max-cycles=") == 0) {
            max_cycles = atoi(arg.c_str()+12);
        }
        if (arg.find("+profile-interval=") == 0) {
            profile_interval = atoi(arg.c_str()+18);
        }
        if (arg.find("+zero-out-dram") == 0) {
            do_zero_out_dram = true;
        }
    }


std::vector<uint64_t> host_mem_offsets;
uint64_t host_mem_offset = -0x80000000LL;
#ifdef MEMMODEL_0
    fpga_models.push_back(new FpgaMemoryModel(
                this,
                // Casts are required for now since the emitted type can change...
                AddressMap(MEMMODEL_0_R_num_registers,
                    (const unsigned int*) MEMMODEL_0_R_addrs,
                    (const char* const*) MEMMODEL_0_R_names,
                    MEMMODEL_0_W_num_registers,
                    (const unsigned int*) MEMMODEL_0_W_addrs,
                    (const char* const*) MEMMODEL_0_W_names),
                argc, argv, "memory_stats0.csv", 1L << TARGET_MEM_ADDR_BITS , host_mem_offset));
     host_mem_offsets.push_back(host_mem_offset);
     host_mem_offset += (1ULL << MEMMODEL_0_target_addr_bits);
#endif

#ifdef MEMMODEL_1
    fpga_models.push_back(new FpgaMemoryModel(
                this,
                // Casts are required for now since the emitted type can change...
                AddressMap(MEMMODEL_1_R_num_registers,
                    (const unsigned int*) MEMMODEL_1_R_addrs,
                    (const char* const*) MEMMODEL_1_R_names,
                    MEMMODEL_1_W_num_registers,
                    (const unsigned int*) MEMMODEL_1_W_addrs,
                    (const char* const*) MEMMODEL_1_W_names),
                argc, argv, "memory_stats1.csv", 1L << TARGET_MEM_ADDR_BITS, host_mem_offset));
     host_mem_offsets.push_back(host_mem_offset);
     host_mem_offset += 1ULL << MEMMODEL_1_target_addr_bits;
#endif

#ifdef MEMMODEL_2
    fpga_models.push_back(new FpgaMemoryModel(
                this,
                // Casts are required for now since the emitted type can change...
                AddressMap(MEMMODEL_2_R_num_registers,
                    (const unsigned int*) MEMMODEL_2_R_addrs,
                    (const char* const*) MEMMODEL_2_R_names,
                    MEMMODEL_2_W_num_registers,
                    (const unsigned int*) MEMMODEL_2_W_addrs,
                    (const char* const*) MEMMODEL_2_W_names),
                argc, argv, "memory_stats2.csv", 1L << TARGET_MEM_ADDR_BITS, host_mem_offset));
     host_mem_offsets.push_back(host_mem_offset);
     host_mem_offset += 1ULL << MEMMODEL_2_target_addr_bits;
#endif

#ifdef MEMMODEL_3
    fpga_models.push_back(new FpgaMemoryModel(
                this,
                // Casts are required for now since the emitted type can change...
                AddressMap(MEMMODEL_3_R_num_registers,
                    (const unsigned int*) MEMMODEL_3_R_addrs,
                    (const char* const*) MEMMODEL_3_R_names,
                    MEMMODEL_3_W_num_registers,
                    (const unsigned int*) MEMMODEL_3_W_addrs,
                    (const char* const*) MEMMODEL_3_W_names),
                argc, argv, "memory_stats3.csv", 1L << TARGET_MEM_ADDR_BITS, host_mem_offset));
     host_mem_offsets.push_back(host_mem_offset);
     host_mem_offset += 1ULL << MEMMODEL_3_target_addr_bits;
#endif

// There can only be one instance of assert and print widgets as their IO is
// uniquely generated by a FIRRTL transform
#ifdef ASSERTIONWIDGET_struct_guard
    #ifdef ASSERTIONWIDGET_0_PRESENT
    ASSERTIONWIDGET_0_substruct_create;
    add_endpoint(new synthesized_assertions_t(this, ASSERTIONWIDGET_0_substruct));
    #endif
#endif

#ifdef PRINTWIDGET_struct_guard
    #ifdef PRINTWIDGET_0_PRESENT
    PRINTWIDGET_0_substruct_create;
    print_endpoint = new synthesized_prints_t(this,
                                          args,
                                          PRINTWIDGET_0_substruct,
                                          PRINTWIDGET_0_print_count,
                                          PRINTWIDGET_0_token_bytes,
                                          PRINTWIDGET_0_idle_cycles_mask,
                                          PRINTWIDGET_0_print_offsets,
                                          PRINTWIDGET_0_format_strings,
                                          PRINTWIDGET_0_argument_counts,
                                          PRINTWIDGET_0_argument_widths,
                                          PRINTWIDGET_0_DMA_ADDR);
    add_endpoint(print_endpoint);
    #endif
#endif
    // Add functions you'd like to periodically invoke on a paused simulator here.
    if (profile_interval != -1) {
        register_task([this](){ return this->profile_models();}, 0);
    }
    // Test harness
    add_endpoint(new test_harness_endpoint_t(this, args));
}

bool fasedtests_top_t::simulation_complete() {
    bool is_complete = false;
    for (auto &e: endpoints) {
        is_complete |= e->terminate();
    }
    return is_complete;
}

uint64_t fasedtests_top_t::profile_models(){
    for (auto mod: fpga_models) {
        mod->profile();
    }
    return profile_interval;
}

int fasedtests_top_t::exit_code(){
    for (auto &e: endpoints) {
        if (e->exit_code())
            return e->exit_code();
    }
    return 0;
}


void fasedtests_top_t::run() {
    for (auto &e: fpga_models) {
        e->init();
    }

    for (auto &e: endpoints) {
        e->init();
    }

    if (do_zero_out_dram) {
        fprintf(stderr, "Zeroing out FPGA DRAM. This will take a few seconds...\n");
        zero_out_dram();
    }
    fprintf(stderr, "Commencing simulation.\n");
    uint64_t start_hcycle = hcycle();
    uint64_t start_time = timestamp();

    // Assert reset T=0 -> 50
    target_reset(0, 50);

    while (!simulation_complete() && !has_timed_out()) {
        run_scheduled_tasks();
        step(get_largest_stepsize(), false);
        while(!done() && !simulation_complete()){
            for (auto &e: endpoints) e->tick();
        }
    }

    uint64_t end_time = timestamp();
    uint64_t end_cycle = actual_tcycle();
    uint64_t hcycles = hcycle() - start_hcycle;
    double sim_time = diff_secs(end_time, start_time);
    double sim_speed = ((double) end_cycle) / (sim_time * 1000.0);
    // always print a newline after target's output
    fprintf(stderr, "\n");
    int exitcode = exit_code();
    if (exitcode) {
        fprintf(stderr, "*** FAILED *** (code = %d) after %llu cycles\n", exitcode, end_cycle);
    } else if (!simulation_complete() && has_timed_out()) {
        fprintf(stderr, "*** FAILED *** (timeout) after %llu cycles\n", end_cycle);
    } else {
        fprintf(stderr, "*** PASSED *** after %llu cycles\n", end_cycle);
    }
    if (sim_speed > 1000.0) {
        fprintf(stderr, "time elapsed: %.1f s, simulation speed = %.2f MHz\n", sim_time, sim_speed / 1000.0);
    } else {
        fprintf(stderr, "time elapsed: %.1f s, simulation speed = %.2f KHz\n", sim_time, sim_speed);
    }
    double fmr = ((double) hcycles / end_cycle);
    fprintf(stderr, "FPGA-Cycles-to-Model-Cycles Ratio (FMR): %.2f\n", fmr);
    expect(!exitcode, NULL);

    for (auto e: fpga_models) {
        e->finish();
    }
#ifdef PRINTWIDGET_0_PRESENT
    print_endpoint->finish();
#endif
}


//
// Created by Mark Plagge on 5/25/16.
//

#include <stdio.h>
#include "nemo_main.h"
#include "./IO/IOStack.h"
#include "./layer_map/layer_map_lib.h"
/** \addtogroup Globals 
 * @{  */

size_type CORES_IN_SIM = 16;
size_type AXONS_IN_CORE = NEURONS_IN_CORE;
size_type SIM_SIZE = 1025;
size_type SYNAPSES_IN_CORE = 0;
size_type CORE_SIZE = 0;
size_type LPS_PER_PE = 0;

bool IS_RAND_NETWORK = true;
bool BULK_MODE = false;
bool DEBUG_MODE = false;
bool SAVE_MEMBRANE_POTS  = false;
bool SAVE_SPIKE_EVTS  = false;
bool SAVE_NEURON_OUTS = false;
bool PHAS_VAL = false;
//bool TONIC_SPK_VAL = false;
bool TONIC_BURST_VAL = false;
bool PHASIC_BURST_VAL = false;
bool VALIDATION = false;

//bool MPI_SAVE = false;
bool BINARY_OUTPUT = false;
unsigned int DO_DUMPI = false;

char * inputFileName = "nemo_in";
char * neuronFireFileName = "fire_record";

unsigned int CORES_IN_CHIP = 4096;
unsigned int CORES_IN_CHIP;
unsigned int NUM_CHIPS_IN_SIM;
unsigned int CHIPS_PER_RANK;
int N_FIRE_BUFF_SIZE = 32;
int N_FIRE_LINE_SIZE = 512;


long double COMPUTE_TIME  = 0.0000000002;
long double SEND_TIME_MIN = 0.0000000001;
long double SEND_TIME_MAX = 0.000000002;

/** @{ sat net flags */
unsigned int SAT_NET_PERCENT = 2;
unsigned int SAT_NET_COREMODE = false;
unsigned int SAT_NET_THRESH = 2;
unsigned int SAT_NET_LEAK = 1;
unsigned int SAT_NET_STOC = false;
unsigned int IS_SAT_NET = false;
/**@} */


/** Layer Network Settings - Globals:
 */

unsigned int NUM_LAYERS_IN_SIM;
layerTypes LAYER_NET_MODE;
unsigned int LAYER_SIZES[4096]; //!< 4k Layers max. Defines the number of neurons in a
unsigned int GRID_ENABLE = 0;
unsigned int GRID_MODE = 0;
unsigned int RND_GRID = 0;
unsigned int RND_UNIQ = 0;
unsigned int UNEVEN_LAYERS = 0;
char * LAYER_LAYOUT;
//
/**
 * @FILE_OUT - is set to true if NeMo is saving output files
 * @FILE_IN - is set to true if NeMo is reading a file.
 * 
 */
bool FILE_OUT = false;
bool FILE_IN = false;

/**
 * outFile - basic output file handler.
 */
FILE *outFile;


int testingMode = 0;


//-----------------------Non global testing vars---------//

char * couchAddress = "192.168.2.3";


/** @} */
/**
 * app_opt - Application Options. Manages the options for NeMo's run.
 */
const tw_optdef app_opt[] = {
	TWOPT_FLAG("rand", IS_RAND_NETWORK, "Generate a random network? Alternatively, "
            "you need to specify config files."),
	TWOPT_FLAG("sat", IS_SAT_NET,"Generate a SAT network with n% core/neuron connectivity"),
	TWOPT_UINT("sp", SAT_NET_PERCENT,"SAT network connectivity percentage in core mode, "
            "or percentage chance of connected axon"),
	TWOPT_UINT("sc", SAT_NET_COREMODE,"SAT network mode: 0: axon probability, 1: Core pool, 2: Neuron pool."),
	TWOPT_UINT("st", SAT_NET_THRESH, "Sat network neuron threshold"),
	TWOPT_UINT("sl", SAT_NET_LEAK, "Sat network per-neuron leak value"),
	TWOPT_FLAG("ss", SAT_NET_STOC,"Sat network stochastic weight mode "),
	TWOPT_UINT("tm", testingMode, "Choose a test suite to run. 0=no tests, 1=mapping tests"),
	TWOPT_GROUP("Randomized (ID Matrix) Network Parameters"),
	TWOPT_UINT("chip", CORES_IN_CHIP, "The number of neurosynaptic cores contained in one chip"),
		TWOPT_ULONGLONG("cores", CORES_IN_SIM, "number of cores in simulation"),
    	//TWOPT_ULONGLONG("neurons", NEURONS_IN_CORE, "number of neurons (and axons) in sim"),
    TWOPT_GROUP("Data Gathering Settings"),

    	TWOPT_FLAG("bulk", BULK_MODE, "Is this sim running in bulk mode?"),
    	TWOPT_FLAG("dbg", DEBUG_MODE, "Debug message printing"),
    	TWOPT_FLAG("network", SAVE_NEURON_OUTS, "Save neuron output axon IDs on creation - Creates a map of the neural network."),
    	TWOPT_FLAG("svm", SAVE_MEMBRANE_POTS, "Save neuron membrane potential values (enabled by default when running a validation model"),
    	TWOPT_FLAG("svs", SAVE_SPIKE_EVTS, "Save neuron spike event times and info"),

    TWOPT_GROUP("Integrated Bio Model Testing"),
    	TWOPT_FLAG("phval", PHAS_VAL, "Phasic Neuron Validation"),
    	TWOPT_FLAG("tonb",TONIC_BURST_VAL, "Tonic bursting Neuron Validation"),
    	TWOPT_FLAG("phb", PHASIC_BURST_VAL, "Phasic Bursting Neuron Validation"),


	TWOPT_GROUP("DUMPI Timing Parameters - All Parameters are in the scale of seconds."),
    TWOPT_FLAG("dmp", DO_DUMPI, "Save simulated DUMPI files. Note: For consistent operation, "
            "run one rank per sim chip."),
		TWOPT_STIME("ct", COMPUTE_TIME, "The time between message sends."
				"\n Used if a collision in TW_NOW is detected when generating DUMPI cmds"),
		TWOPT_STIME("stmin", SEND_TIME_MIN, "The send time min value"),
		TWOPT_STIME("stmax", SEND_TIME_MAX, "The maximum send time value"),
	TWOPT_GROUP("Layer/Grid Network Benchmark Parameters"),
	TWOPT_FLAG("gd", GRID_ENABLE,"Enable Grid Mode"),
	TWOPT_UINT("gm", GRID_MODE, "Grid Mode: 0 is Linear, 1 is Convolutional"),
	TWOPT_FLAG("gr", RND_GRID , "Enable random grid mode"),
	TWOPT_FLAG("gru", RND_UNIQ , "For random grids, are connections unique (no neurons attached to the same axon)"),
    TWOPT_UINT("lnum", NUM_LAYERS_IN_SIM, "Number of layers in simulation. Ignored if grid mode."),
	TWOPT_UINT("gridsize", CHIPS_PER_LAYER, "Number of chips in a layer. Grid mode only. Must be evenly distributable,"),
	TWOPT_FLAG("uneven", UNEVEN_LAYERS, "Enable uneven layer mode. Must specify chips per layer in CPL option"),

	TWOPT_END()

};


/**
 * model_lps - contains the LP type defs for NeMo
 */
tw_lptype model_lps[] = {
	{

        (init_f)axon_init,
        (pre_run_f)NULL,
        (event_f)axon_event,
        (revent_f)axon_reverse,
        (commit_f) axon_commit,
        (final_f)axon_final,
        (map_f)getPEFromGID,
        sizeof(axonState) },
    {
        (init_f)synapse_init, 
        (pre_run_f)NULL,
        (event_f)synapse_event,
        (revent_f)synapse_reverse,
        (commit_f) NULL,
        (final_f)NULL,
        (map_f)getPEFromGID, 
        sizeof(synapseState)
    },
    {
        (init_f)TN_init,
        (pre_run_f)NULL,
        (event_f)TN_forward_event,
        (revent_f)TN_reverse_event,
        (commit_f) TN_commit	,
        (final_f)TN_final,
        (map_f)getPEFromGID, 
        sizeof(tn_neuron_state)
    }
    ,
        { 0 } };

/**
 * @brief      Displays NeMo's initial run size configuration.
 */
void displayModelSettings()
{
//	char *header = { '*'
//    if(g_tw_mynode == 0){
//    for (int i = 0; i < 30; i++)
//    {
//        printf("*");
//    }
	TH
    double cores_per_node = CORES_IN_SIM / tw_nnodes() ;
    char *netMode = FILE_IN ? "file defined":"random benchmark";
    printf("\n");
    printf("* \t %i Neurons per core (cmake defined), %llu cores in sim.\n", NEURONS_IN_CORE, CORES_IN_SIM);
    printf("* \t %f cores per PE, giving %llu LPs per pe.\n", cores_per_node, g_tw_nlp);
    printf("* \t Neurons have %i axon types (cmake defined)\n", NUM_NEURON_WEIGHTS);
    printf("* \t Network is a %s network.\n",netMode);
    printf("* \t Neuron stats:\n");
    printf("* \tCalculated sim_size is %llu\n", SIM_SIZE);
    printf("* \tSave Messages: %i \n", SAVE_MSGS );
	TH
	printf("* \tChip Sim Info:\n");
	printf("* \tCores per chip: %i\n", CORES_IN_CHIP);
	printf("* \tReported chips in sim: %li\n", coreToChip(CORES_IN_SIM  ));
    TH
    STT("SAT NET ENABLED: %i", IS_SAT_NET);
    STT("SAT net stoc. mode: %i", SAT_NET_STOC);
    STT("SAT NET Weight: %u %%", SAT_NET_PERCENT);
    STT("SAT mode set to %u ", SAT_NET_COREMODE)
    printf("* \t Modes: (0) - Neuron %%, (1) - Core Pool, (2) Neuron Pool \n");
    TH
	printf("* \tLayer Network Parameters \n");
    displayConfig();
	printf("\n");
//    unsigned int SAT_NET_PERCENT = 2;
//    bool SAT_NET_COREMODE = false;
//    unsigned int SAT_NET_THRESH = 2;
//    unsigned int SAT_NET_LEAK = 1;
//    bool SAT_NET_STOC = false;
//    bool IS_SAT_NET = false;
}
/** @brief Does initial tests of Neuron Output subsystem.
 * If subsystem tests are on, then this will "simulate" a series of neuron firing events after
 * initializing file systems.
 *
 * Tests file closing function as well.
 */

void testNeuronOut(){
    SAVE_SPIKE_EVTS = true;
	initOutFiles();

    for (int i = 0; i < 4096; i ++){
        saveNeuronFire(random() + i, 0,0,1024);
    }
    closeFiles();

}

/**
 * @brief      Initializes NeMo
 * 
 * First, this function checks for potential file IO, and creates file handles for use.
 * 
 * Based on the file_in option, the function then sets the neuron, axon, and synapse 
 * function pointers to the proper values. Default is the IBM TrueNorth neuron model.
 * If NeMo is reading a file, then we set the size of the sim based on the model config file.
 * 
 * The rest of this function manages ROSS initialization and setup. When done,
 * 
 */
void init_nemo(){
    if (SAT_NET_COREMODE > 2){
        tw_error(TW_LOC, "Please choose a valid SAT mode if using SAT. \n"
                "Can be 0,1,2");
    }
	VALIDATION = PHAS_VAL || TONIC_BURST_VAL || PHASIC_BURST_VAL;
	FILE_OUT = SAVE_SPIKE_EVTS || SAVE_NEURON_OUTS || 
				SAVE_MEMBRANE_POTS || VALIDATION;

	FILE_IN = !IS_RAND_NETWORK;
    FILE_IN = false;

	if (FILE_OUT){
		//Init file output handles
		initOutFiles();
		if(g_tw_mynode == 0){
			printf("Output Files Init.\n");
		}
	}

	if (FILE_IN){
		//Init File Input Handles
		//reconfigure cores_in_sim and neurons_in_sim based on loaded file.
		//override default LP function pointers
		
	}

	
	AXONS_IN_CORE = NEURONS_IN_CORE;
	SYNAPSES_IN_CORE = 1;//(NEURONS_IN_CORE * AXONS_IN_CORE);
    
	CORE_SIZE = SYNAPSES_IN_CORE + NEURONS_IN_CORE + AXONS_IN_CORE;
	SIM_SIZE = CORE_SIZE * CORES_IN_SIM;

	g_tw_nlp = SIM_SIZE / tw_nnodes();
	g_tw_lookahead = 0.001;
    g_tw_lp_types = model_lps;
    g_tw_lp_typemap = lpTypeMapper;



	///EVENTS PER PE SETTING
	g_tw_events_per_pe = NEURONS_IN_CORE * AXONS_IN_CORE ; //magic number
                               
    LPS_PER_PE = g_tw_nlp / g_tw_npe;

	NUM_CHIPS_IN_SIM = CORES_IN_SIM / CORES_IN_CHIP;
	CHIPS_PER_RANK = 1;
    //Layer / Grid Mode setup:
    setupGrid(0);

}



unsigned char mapTests(){
    return 0;
}

/**
 * @brief      NeMo Main entry point
 *
 * @param[in]  argc  The argc
 * @param      argv  The argv
 *
 
 */
int main(int argc, char*argv[]) {
	tw_opt_add(app_opt);
	tw_init(&argc, &argv);
    //call nemo init
    init_nemo();



	
    if (testingMode == 1 ) {
        unsigned char mapResult = 0;
        mapResult = mapResult | mapTests();

        if(mapResult & INVALID_AXON){
            printf("Creted invalid axon.\n");
            }
        if (mapResult & INVALID_SYNAPSE){
            printf("Created invalid synapse.\n");
        }
        if (mapResult & INVALID_NEURON){
            printf("Created invalid neuron.\n");
        }


        return mapResult;
    }
    //Define LPs:
    tw_define_lps(LPS_PER_PE, sizeof(messageData));
    tw_lp_setup_types();

    if (g_tw_mynode == 0) {
        displayModelSettings();
    }
    //neuron fire output testing function.
	//testNeuronOut();
	
    tw_run();
	
	if(FILE_OUT)
		printf("File output complete.");
		//closeFiles();
	if(FILE_IN)
		printf("File input set but not implemented yet.");
	
    tw_end();
	

}

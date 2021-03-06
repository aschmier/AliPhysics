AliAnalysisGrid* CreateAlienHandler(Bool_t bUseParFiles=kFALSE) {
  // Check if user has a valid token, otherwise make one. This has limitations.
  // One can always follow the standard procedure of calling alien-token-init 
  // then source /tmp/gclient_env_$UID in the current shell.
  AliAnalysisAlien *plugin = new AliAnalysisAlien();
  
  // Set the run mode (can be "full", "test", "offline", "submit" or "terminate")
  plugin->SetRunMode("test");
  //plugin->SetRunMode("offline");
  //plugin->SetRunMode("submit");
  //plugin->SetRunMode("full");
  //plugin->SetRunMode("terminate");
  plugin->SetNtestFiles(1); // Relevant only for run mode "test" 

  // Set versions of used packages
  plugin->SetAPIVersion("V1.1x");
  //plugin->SetROOTVersion("v5-27-06b");
  //plugin->SetAliROOTVersion("v4-21-13-AN");  
  //plugin->SetAliPhysicsVersion("vAN-20191031_ROOT6-1");
  plugin->SetAliPhysicsVersion("vAN-20200724-1");
  
  // Declare input data to be processed - can be done in two ways:
  // METHOD 1: Create automatically XML collections using alien 'find' command.
  // ============================================================================
  //  Example 1: MC production (set in macro runFlowTask.C: DATA = kFALSE)
  //plugin->SetGridDataDir("/alice/sim/LHC10d4");
  //plugin->SetDataPattern("*AliESDs.root"); // The default data pattern, other may be "*tag.root", "*ESD.tag.root", etc
  //plugin->AddRunNumber(119844); // Alternatively use e.g. plugin->SetRunRange(105044,106044); to add more runs in one go  
  //plugin->SetOutputToRunNo(); 
  // ============================================================================
  //  Example 2: Real data (set in macro runFlowTask.C: DATA = kTRUE, MCEP = kFALSE)
  //plugin->SetGridDataDir("/alice/data/2010/LHC10h");
  plugin->SetGridDataDir("/alice/data/2015/LHC15o");  // 2015
  //plugin->SetDataPattern("*ESDs/pass1_5plus/*ESDs.root");
  //plugin->SetDataPattern("ESDs/pass2/AOD160/*AliAOD.root");
  plugin->SetDataPattern("/pass1/AOD194/*AliAOD.root"); //2015
  //plugin->SetDataPattern("/pass1/15000246994019.101*/AliESDs.root");
  plugin->SetRunPrefix("000"); // IMPORTANT!
  //plugin->AddRunNumber(137161); 

  //Run List
  const int nrun = 3;
  int runs[nrun] = {
    246994, 246991, 246989  //, 246984, 246982, //138275
  };

  for (int i(0); i < nrun; ++i) plugin->AddRunNumber(runs[i]);
  
  //plugin->AddRunNumber(246994);
  // plugin->AddRunNumber(119844); // Alternatively use e.g. plugin->SetRunRange(104044,106044); to add more runs in one go 
  plugin->SetOutputToRunNo(1);  
  // ============================================================================
 
  // METHOD 2: Declare existing data files (raw collections, xml collections, root file)
  // If no path mentioned data is supposed to be in the work directory (see SetGridWorkingDir())
  // XML collections added via this method can be combined with the first method if
  // the content is compatible (using or not tags)
  //plugin->AddDataFile("hijingWithoutFlow10000Evts.xml");
  //   plugin->AddDataFile("/alice/data/2008/LHC08c/000057657/raw/Run57657.Merged.RAW.tag.root");
  // Define alien work directory where all files will be copied. Relative to alien $HOME.
  //plugin->SetGridWorkingDir("data");
  //plugin->SetGridWorkingDir("wdir_LHC2015o_246994");
  //plugin->SetGridWorkingDir("wdir_LHC2015o_246994_246991_246989_AddTaskFlowQC_AOD_pt_0_3_cent_0_30_WeightsOn"); //wdir_LHC2015o_246994_246991_AddTaskFlowQC_AOD_pt_0_3
  plugin->SetGridWorkingDir("wdir_LHC2015o_246994_246991_246989_AddTaskCRC_AOD_pt_0_3");
  // Declare alien output directory. Relative to working directory.
  //plugin->SetGridOutputDir("output1_5plus"); // In this case will be $HOME/work/output
  //plugin->SetGridOutputDir("output_LHC2015o_246994_246991_246989_AddTaskFlowQC_AOD_pt_0_3_cent_0_30_WeightsOn"); // In this case will be $HOME/work/output
  plugin->SetGridOutputDir("output_LHC2015o_246994_246991_246989_AddTaskCRC_AOD_pt_0_3");
  // Declare the analysis source files names separated by blancs. To be compiled runtime
  // using ACLiC on the worker nodes:
  // ... (if this is needed see in official tutorial example how to do it!)

  // Declare all libraries (other than the default ones for the framework. These will be
  // loaded by the generated analysis macro. Add all extra files (task .cxx/.h) here.
  //plugin->SetAdditionalLibs("libCORRFW.so libTOFbase.so libTOFrec.so");
  if(!bUseParFiles)
  {
    plugin->SetAdditionalLibs("libGui.so libProof.so libMinuit.so libXMLParser.so "
                              "libRAWDatabase.so libRAWDatarec.so libCDB.so libSTEERBase.so "
                              "libSTEER.so libTPCbase.so libTOFbase.so libTOFrec.so "
                              "libTRDbase.so libVZERObase.so libVZEROrec.so libT0base.so "
                              "libT0rec.so libTender.so libTenderSupplies.so "
                              "libPWGflowBase.so libPWGflowTasks.so");
  } 
  else // load libs via par files
  { 
    plugin->SetAdditionalLibs("libGui.so  libProof.so  libMinuit.so  libXMLParser.so "
                              "libRAWDatabase.so  libRAWDatarec.so  libCDB.so  libSTEERBase.so "
                              "libSTEER.so  libTPCbase.so  libTOFbase.so  libTOFrec.so "
                              "libTRDbase.so  libVZERObase.so  libVZEROrec.so  libT0base.so "
                              "libT0rec.so  libTender.so  libTenderSupplies.so");
    plugin->EnablePackage("PWGflowBase.par");
    plugin->EnablePackage("PWGflowTasks.par");
  }
  // Do not specify your outputs by hand anymore:
  plugin->SetDefaultOutputs(kTRUE);
  // To specify your outputs by hand set plugin->SetDefaultOutputs(kFALSE); and comment in line plugin->SetOutputFiles("..."); 
  // and plugin->SetOutputArchive("..."); bellow.
  // Declare the output file names separated by blancs.
  // (can be like: file.root or file.root@ALICE::Niham::File)
  // plugin->SetOutputFiles("AnalysisResults.root");
  // Optionally define the files to be archived.
  // plugin->SetOutputArchive("log_archive.zip:stdout,stderr@ALICE::NIHAM::File root_archive.zip:*.root@ALICE::NIHAM::File");
  // plugin->SetOutputArchive("log_archive.zip:stdout,stderr");
  // plugin->SetOutputArchive("log_archive.zip:");
  // Optionally set a name for the generated analysis macro (default MyAnalysis.C)
  plugin->SetAnalysisMacro("flowAnalysis.C");
  // Optionally set maximum number of input files/subjob (default 100, put 0 to ignore)
  plugin->SetSplitMaxInputFileNumber(80);
  // Optionally set number of runs per masterjob:
  plugin->SetNrunsPerMaster(1);
  // Optionally set overwrite mode. Will trigger overwriting input data colections AND existing output files:
  plugin->SetOverwriteMode(kTRUE);
  // Optionally set number of failed jobs that will trigger killing waiting sub-jobs.
  plugin->SetMaxInitFailed(5);
  // Optionally resubmit threshold.
  plugin->SetMasterResubmitThreshold(90);
  // Optionally set time to live (default 30000 sec)
  plugin->SetTTL(86400);
  // Optionally set input format (default xml-single)
  plugin->SetInputFormat("xml-single");
  // Optionally modify the name of the generated JDL (default analysis.jdl)
  plugin->SetJDLName("flowAnalysis.jdl");
  // Optionally modify job price (default 1)
  plugin->SetPrice(1);      
  // Optionally modify split mode (default 'se')    
  plugin->SetSplitMode("se");
  
  //plugin->SetMergeViaJDL(kTRUE);
  //plugin->SetOneStageMerging(kFALSE);
  //plugin->SetMaxMergeStages(10);
  //plugin->SetMaxMergeFiles(20);

  return plugin;
}

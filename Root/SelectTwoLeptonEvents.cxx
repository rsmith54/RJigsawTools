#include <EventLoop/Job.h>
#include <EventLoop/StatusCode.h>
#include <EventLoop/Worker.h>
#include <RJigsawTools/SelectTwoLeptonEvents.h>

#include <AsgTools/MsgStream.h>
#include <AsgTools/MsgStreamMacros.h>

#include "xAODRootAccess/Init.h"
#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/TStore.h"

#include "SUSYTools/SUSYObjDef_xAOD.h"

#include <RJigsawTools/strongErrorCheck.h>

#include "xAODParticleEvent/ParticleContainer.h"
#include "xAODParticleEvent/ParticleAuxContainer.h"
#include <utility>

// this is needed to distribute the algorithm to the workers
ClassImp(SelectTwoLeptonEvents)



SelectTwoLeptonEvents :: SelectTwoLeptonEvents ()
{
  // Here you put any code for the base initialization of variables,
  // e.g. initialize all pointers to 0.  Note that you should only put
  // the most basic initialization here, since this method will be
  // called on both the submission and the worker node.  Most of your
  // initialization code will go into histInitialize() and
  // initialize().
}



EL::StatusCode SelectTwoLeptonEvents :: setupJob (EL::Job& job)
{
  // Here you put code that sets up the job on the submission object
  // so that it is ready to work with your algorithm, e.g. you can
  // request the D3PDReader service or add output files.  Any code you
  // put here could instead also go into the submission script.  The
  // sole advantage of putting it here is that it gets automatically
  // activated/deactivated when you add/remove the algorithm from your
  // job, which may or may not be of value to you.
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode SelectTwoLeptonEvents :: histInitialize ()
{
  // Here you do everything that needs to be done at the very
  // beginning on each worker node, e.g. create histograms and output
  // trees.  This method gets called before any input files are
  // connected.
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode SelectTwoLeptonEvents :: fileExecute ()
{
  // Here you do everything that needs to be done exactly once for every
  // single file, e.g. collect a list of all lumi-blocks processed
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode SelectTwoLeptonEvents :: changeInput (bool firstFile)
{
  // Here you do everything you need to do when we change input files,
  // e.g. resetting branch addresses on trees.  If you are using
  // D3PDReader or a similar service this method is not needed.
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode SelectTwoLeptonEvents :: initialize ()
{
  // Here you do everything that you need to do after the first input
  // file has been connected and before the first event is processed,
  // e.g. create additional histograms based on which variables are
  // available in the input files.  You can also create all of your
  // histograms and trees in here, but be aware that this method
  // doesn't get called if no events are processed.  So any objects
  // you create here won't be available in the output if you have no
  // input events.
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode SelectTwoLeptonEvents :: execute ()
{
  // Here you do everything that needs to be done on every single
  // events, e.g. read input variables, apply cuts, and fill
  // histograms and trees.  This is where most of your actual analysis
  // code will go.

  //todo add some preselection here!

  xAOD::TStore * store = wk()->xaodStore();

  const xAOD::EventInfo* eventInfo = 0;
  STRONG_CHECK(store->retrieve( eventInfo, "EventInfo"));

  // If the event didn't pass the preselection alg, don't bother doing anything with it...
  std::string preselectedRegionName =  eventInfo->auxdecor< std::string >("regionName");
  ATH_MSG_DEBUG("Preselected?: " << preselectedRegionName  );

  if( preselectedRegionName == "" ) return EL::StatusCode::SUCCESS;


  std::pair<xAOD::ParticleContainer* , xAOD::ParticleAuxContainer*> selectedLeptons( new xAOD::ParticleContainer() , new xAOD::ParticleAuxContainer);
  selectedLeptons.first->setStore(selectedLeptons.second);
  std::pair<xAOD::ParticleContainer* , xAOD::ParticleAuxContainer*> selectedJets   ( new xAOD::ParticleContainer() , new xAOD::ParticleAuxContainer);
  selectedJets.first->setStore(selectedJets.second);
  std::pair<xAOD::ParticleContainer* , xAOD::ParticleAuxContainer *> selectedRJigsawParticles(new xAOD::ParticleContainer, new xAOD::ParticleAuxContainer);
  selectedRJigsawParticles.first->setStore(selectedRJigsawParticles.second);

  STRONG_CHECK( store->record( selectedLeptons.first  , "selectedLeptons"    ) );//todo configurable if needed
  STRONG_CHECK( store->record( selectedLeptons.second , "selectedLeptonsAux."    ) );//todo configurable if needed
  STRONG_CHECK( store->record( selectedJets.first     , "selectedJets"    ) );//todo configurable if needed
  STRONG_CHECK( store->record( selectedJets.second    , "selectedJetsAux."    ) );//todo configurable if needed

  STRONG_CHECK( store->record( selectedRJigsawParticles.first  , "myparticles"    ) );//todo configurable if needed
  STRONG_CHECK( store->record( selectedRJigsawParticles.second , "myparticlesaux."    ) );//todo configurable if needed




  xAOD::JetContainer* jets_nominal(nullptr);
  STRONG_CHECK(store->retrieve(jets_nominal, "STCalibAntiKt4EMTopoJets"));

  xAOD::MuonContainer* muons_nominal(nullptr);
  STRONG_CHECK(store->retrieve(muons_nominal, "STCalibMuons"));

  xAOD::ElectronContainer* electrons_nominal(nullptr);
  STRONG_CHECK(store->retrieve(electrons_nominal, "STCalibElectrons"));

  for (const auto& jet : *jets_nominal) {
    if ((int)jet->auxdata<char>("baseline") == 0) continue;
    if ((int)jet->auxdata<char>("passOR") != 1) continue;
    if ((int)jet->auxdata<char>("signal") != 1) continue;
    // If I've gotten this far, I have a signal, isolated, beautiful jet
    ATH_MSG_VERBOSE( "jet pt : " << jet->pt() );

    auto tmpparticle = new xAOD::Particle();
    selectedJets.first->push_back(tmpparticle  );
    tmpparticle->setP4(jet->p4());
  }

  for (const auto& mu : *muons_nominal) {
    if ((int)mu->auxdata<char>("baseline") == 0) continue;
    if ((int)mu->auxdata<char>("passOR") != 1) continue;
    if ((int)mu->auxdata<char>("signal") != 1) continue;
    // If I've gotten this far, I have a signal, isolated, beautiful muon
    ATH_MSG_VERBOSE( "mu pt : " << mu->pt() );

    auto tmpparticle = new xAOD::Particle();
    selectedLeptons.first->push_back(tmpparticle  );
    tmpparticle->setP4(mu->p4());
  }

  for (const auto& el : *electrons_nominal) {
    if ((int)el->auxdata<char>("baseline") == 0) continue;
    if ((int)el->auxdata<char>("passOR") != 1) continue;
    if ((int)el->auxdata<char>("signal") != 1) continue;
    // If I've gotten this far, I have a signal, isolated, beautiful el
    ATH_MSG_VERBOSE( "el pt : " << el->pt() );

    auto tmpparticle = new xAOD::Particle();
    selectedLeptons.first->push_back(tmpparticle  );
    tmpparticle->setP4(el->p4());
  }

  int const nLeptons = selectedLeptons.first->size();

  ATH_MSG_DEBUG("Number of Selected Leptons: " << nLeptons  );

  //Let's just categorize from here maybe? But if we want different CRs in different algs,
  // then we'd need to play with something in the store a little more smartly

  std::string regionName = "";

  auto  getRegionName = [](int nLeptons){ std::string regionName = "";
					  if(nLeptons == 2){regionName = "SR";}
					  if(nLeptons == 1){regionName = "CR1L";}
					  if(nLeptons == 0){regionName = "CR0L";}
					  return regionName;};//todo >2 leptons in the SR? pretty rare though


  //Here we should add the particles that we want in the calculation to myparticles
  for( const auto& mylepton: *selectedLeptons.first){
    xAOD::Particle *tmpparticle = new xAOD::Particle;
    selectedRJigsawParticles.first->push_back(tmpparticle  );
    tmpparticle->setP4( mylepton->p4() );
  }

  // // What happens if we add the jets into the calculation?
  for( const auto& myjet: *selectedJets.first){
    xAOD::Particle *tmpparticle = new xAOD::Particle;
    selectedRJigsawParticles.first->push_back(tmpparticle  );
    tmpparticle->setP4( myjet->p4() );
  }

  ATH_MSG_DEBUG("Event falls in region: " << getRegionName( nLeptons)  );

  ATH_MSG_DEBUG("Writing particle container for calculator to store");

  eventInfo->auxdecor< std::string >("regionName") = getRegionName( nLeptons) ;
  ATH_MSG_DEBUG("Writing to eventInfo decoration: " <<  eventInfo->auxdecor< std::string >("regionName")   );

  return EL::StatusCode::SUCCESS;
}



EL::StatusCode SelectTwoLeptonEvents :: postExecute ()
{
  // Here you do everything that needs to be done after the main event
  // processing.  This is typically very rare, particularly in user
  // code.  It is mainly used in implementing the NTupleSvc.
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode SelectTwoLeptonEvents :: finalize ()
{
  // This method is the mirror image of initialize(), meaning it gets
  // called after the last event has been processed on the worker node
  // and allows you to finish up any objects you created in
  // initialize() before they are written to disk.  This is actually
  // fairly rare, since this happens separately for each worker node.
  // Most of the time you want to do your post-processing on the
  // submission node after all your histogram outputs have been
  // merged.  This is different from histFinalize() in that it only
  // gets called on worker nodes that processed input events.
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode SelectTwoLeptonEvents :: histFinalize ()
{
  // This method is the mirror image of histInitialize(), meaning it
  // gets called after the last event has been processed on the worker
  // node and allows you to finish up any objects you created in
  // histInitialize() before they are written to disk.  This is
  // actually fairly rare, since this happens separately for each
  // worker node.  Most of the time you want to do your
  // post-processing on the submission node after all your histogram
  // outputs have been merged.  This is different from finalize() in
  // that it gets called on all worker nodes regardless of whether
  // they processed input events.
  return EL::StatusCode::SUCCESS;
}
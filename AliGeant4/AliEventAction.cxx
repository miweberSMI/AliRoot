// $Id$
// Category: event
//
// Author: I. Hrivnacova
//
// Class AliEventAction
// ---------------------
// See the class description in the header file.

#include <G4Timer.hh>
   // in order to avoid the odd dependency for the
   // times system function this include must be the first

#include "AliEventAction.h"
#include "AliTrackingAction.h"
#include "AliGlobals.h"
#include "AliRun.h"
#include "AliHeader.h"

#include <G4Event.hh>
#include <G4TrajectoryContainer.hh>
#include <G4Trajectory.hh>
#include <G4VVisManager.hh>
#include <G4UImanager.hh>

//_____________________________________________________________________________
AliEventAction::AliEventAction()
  : AliVerbose("eventAction"),
    fMessenger(this),
    fDrawFlag("CHARGED")
{
//
  fTimer = new G4Timer();
}

//_____________________________________________________________________________
AliEventAction::AliEventAction(const AliEventAction& right)
  : AliVerbose(""),
    fMessenger(this) {
//
  AliGlobals::Exception("AliEventAction is protected from copying.");
}

//_____________________________________________________________________________
AliEventAction::~AliEventAction() {
//
  delete fTimer;
}

// operators

//_____________________________________________________________________________
AliEventAction& AliEventAction::operator=(const AliEventAction &right)
{
  // check assignement to self
  if (this == &right) return *this;
  
  AliGlobals::Exception("AliEventAction is protected from assigning.");

  return *this;
}

// private methods

//_____________________________________________________________________________
void AliEventAction::DisplayEvent(const G4Event* event) const
{
// Draws trajectories.
// ---

  if (G4VVisManager::GetConcreteInstance()) {

    // trajectories processing
    G4TrajectoryContainer* trajectoryContainer 
      = event->GetTrajectoryContainer();

    G4int nofTrajectories = 0;
    if (trajectoryContainer)
      nofTrajectories = trajectoryContainer->entries(); 
  
    if (VerboseLevel() > 0 && nofTrajectories > 0) {
      G4cout << "    " << nofTrajectories; 
      G4cout << " trajectories stored." << G4endl;
    }  

    for (G4int i=0; i<nofTrajectories; i++) { 
      G4VTrajectory* vtrajectory = (*(event->GetTrajectoryContainer()))[i];
      G4Trajectory* trajectory = dynamic_cast<G4Trajectory*>(vtrajectory);
      if (!trajectory) {
        AliGlobals::Exception(
	  "AliEventAction::DisplayEvent: Unknown trajectory type.");
      }
      if ( (fDrawFlag == "ALL") ||
          ((fDrawFlag == "CHARGED") && (trajectory->GetCharge() != 0.))){
	 trajectory->DrawTrajectory(50); 
	    // the argument number defines the size of the step points
	    // use 2000 to make step points well visible
      }	
    }      
  }
}

// public methods

//_____________________________________________________________________________
void AliEventAction::BeginOfEventAction(const G4Event* event)
{
// Called by G4 kernel at the beginning of event.
// ---

  G4int eventID = event->GetEventID();

  // reset the tracks counters
  if(AliTrackingAction::Instance()) 
    AliTrackingAction::Instance()->PrepareNewEvent();   

  if (VerboseLevel() > 0) {
    G4cout << ">>> Event " << event->GetEventID() << G4endl;
  }  

  fTimer->Start();
}

//_____________________________________________________________________________
void AliEventAction::EndOfEventAction(const G4Event* event)
{
// Called by G4 kernel at the end of event.
// ---

  // finish the last primary track of the current event
  AliTrackingAction* trackingAction = AliTrackingAction::Instance();
  if (trackingAction) trackingAction->FinishPrimaryTrack();   

  if (VerboseLevel() > 0) {
    G4cout << G4endl;
    G4cout << ">>> End of Event " << event->GetEventID() << G4endl;
  }

  if (VerboseLevel() > 1) {
    //G4int nofPrimaryTracks = trackingAction->GetNofPrimaryTracks();
    G4int nofPrimaryTracks = gAlice->GetHeader()->GetNprimary();
    G4int nofSavedTracks = gAlice->GetNtrack();
   
    G4cout  << "    " << nofPrimaryTracks << 
               " primary tracks processed." << G4endl;
    G4cout  << "    " << nofSavedTracks << 
               " tracks saved." << G4endl;
    if (trackingAction) {
       G4int nofAllTracks = trackingAction->GetNofTracks();
       G4cout  << "    " << nofAllTracks << 
                  " all tracks processed." << G4endl;
    }	  
  }	       

  // display event
  DisplayEvent(event);

  // aliroot finish event
  gAlice->FinishEvent();    

  if (VerboseLevel() > 1) {
    // print time
    fTimer->Stop();
    G4cout << "Time of this event: " << *fTimer << G4endl;
  }  
 }

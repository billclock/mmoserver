/*
---------------------------------------------------------------------------------------
This source file is part of SWG:ANH (Star Wars Galaxies - A New Hope - Server Emulator)

For more information, visit http://www.swganh.com

Copyright (c) 2006 - 2010 The SWG:ANH Team
---------------------------------------------------------------------------------------
Use of this source code is governed by the GPL v3 license that can be found
in the COPYING file or at http://www.gnu.org/licenses/gpl-3.0.html

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
---------------------------------------------------------------------------------------
*/
#include "ObjectController.h"
#include "ObjectControllerOpcodes.h"
#include "ObjectControllerCommandMap.h"
#include "PlayerObject.h"
#include "StateManager.h"
#include "QTRegion.h"
#include "CellObject.h"
#include "WorldManager.h"
#include "QuadTree.h"
#include "ZoneTree.h"
#include "NetworkManager/Message.h"


//=============================================================================
//
// sit
//

void ObjectController::_handleSitServer(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{    
    PlayerObject*	playerObject	= dynamic_cast<PlayerObject*>(mObject);

    if(playerObject)
    {
        BString			data;
        glm::vec3       chair_position;
        uint64			chairCell		= 0;
        uint32			elementCount	= 0;

        if(playerObject->checkPlayerCustomFlag(PlayerCustomFlag_LogOut))
        {
            playerObject->togglePlayerFlagOff(PlayerCustomFlag_LogOut);	
            gMessageLib->SendSystemMessage(::common::OutOfBand("logout", "aborted"), playerObject);
        }

        // see if we need to get out of sampling mode
        if(playerObject->getSamplingState())
        {
            gMessageLib->SendSystemMessage(::common::OutOfBand("survey", "sample_cancel"), playerObject);
            playerObject->setSamplingState(false);
        }
        
        message->getStringUnicode16(data); //Should be okay even if data is null! (I hope)  

        // sitting on chair
        if(data.getLength())
        {
            elementCount = swscanf(data.getUnicode16(), L"%f,%f,%f,%"WidePRIu64, &chair_position.x, &chair_position.y, &chair_position.z, &chairCell);

            if(elementCount == 4)
            {
                // outside
                if(!chairCell)
                {
                    if(QTRegion* newRegion = gWorldManager->getSI()->getQTRegion(chair_position.x, chair_position.z))
                    {
                        // we didnt change so update the old one
                        if((uint32)newRegion->getId() == playerObject->getSubZoneId())
                        {
                            // playerObject also updates the players position
                            newRegion->mTree->updateObject(playerObject, chair_position);
                        }
                        else
                        {
                            // remove from old
                            if(QTRegion* oldRegion = gWorldManager->getQTRegion(playerObject->getSubZoneId()))
                            {
                                oldRegion->mTree->removeObject(playerObject);
                            }

                            // update players position
                            playerObject->mPosition = chair_position;

                            // put into new
                            playerObject->setSubZoneId((uint32)newRegion->getId());
                            newRegion->mTree->addObject(playerObject);
                        }
                    }
                    else
                    {
                        // we should never get here !
                        DLOG(INFO) << "SitOnObject: could not find zone region in map";

                        // hammertime !
                        exit(-1);
                    }
                }
                // we are in a cell
                else
                {
                    // switch cells, if needed
                    if(playerObject->getParentId() != chairCell)
                    {
                        CellObject* cell = dynamic_cast<CellObject*>(gWorldManager->getObjectById(playerObject->getParentId()));

                        if(cell)
                            cell->removeObject(playerObject);
                        else
                            DLOG(INFO) << "Error removing " << playerObject->getId() <<" to cell " << chairCell;

                        playerObject->setParentId(chairCell);

                        cell = dynamic_cast<CellObject*>(gWorldManager->getObjectById(chairCell));

                        if(cell)
                            cell->addObjectSecure(playerObject);
                        else
                            DLOG(INFO) << "Error adding " << playerObject->getId() <<" to cell " << chairCell;
                    }

                    playerObject->mPosition = chair_position;
                }

                //playerObject->mDirection = Anh_Math::Quaternion();
                gStateManager.setCurrentActionState(playerObject, CreatureState_SittingOnChair);

                // TODO: check if we need to send transforms to others
                if(chairCell)
                {
                    gMessageLib->sendDataTransformWithParent053(playerObject);
                }
                else
                {
                    gMessageLib->sendDataTransform053(playerObject);
                }
                gMessageLib->sendSitOnObject(playerObject);
            }
        }
        // sitting on ground
        else
        {
            //gMessageLib->sendPostureUpdate(playerObject);
            //gMessageLib->sendSelfPostureUpdate(playerObject);
        }
        gStateManager.setCurrentPostureState(playerObject, CreaturePosture_Sitting);
    }
}

//=============================================================================
//
// stand
//

void ObjectController::_handleStand(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
    PlayerObject*	playerObject = dynamic_cast<PlayerObject*>(mObject);

    if(playerObject)
        gStateManager.setCurrentPostureState(playerObject, CreaturePosture_Upright);
}

//=============================================================================
//
// prone
//

void ObjectController::_handleProne(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
    PlayerObject*	playerObject	= dynamic_cast<PlayerObject*>(mObject);

    if(playerObject)
        gStateManager.setCurrentPostureState(playerObject, CreaturePosture_Prone);
}

//=============================================================================
//
// kneel
//

void ObjectController::_handleKneel(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
    PlayerObject*	playerObject = dynamic_cast<PlayerObject*>(mObject);

    if(playerObject) 
        gStateManager.setCurrentPostureState(playerObject, CreaturePosture_Crouched);
}

//=============================================================================




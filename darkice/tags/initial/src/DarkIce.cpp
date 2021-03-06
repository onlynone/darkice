/*------------------------------------------------------------------------------

   Copyright (c) 2000 Tyrell Corporation. All rights reserved.

   Tyrell DarkIce

   File     : darkice.cpp
   Version  : $Revision$
   Author   : $Author$
   Location : $Source$
   
   Abstract : 

     Program main object

   Copyright notice:

     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License  
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.
    
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of 
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
     GNU General Public License for more details.
    
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
     USA.

------------------------------------------------------------------------------*/

/* ============================================================ include files */

#include <unistd.h>
#include <stdlib.h>
#include <iostream.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "DarkIce.h"


/* ===================================================  local data structures */


/* ================================================  local constants & macros */

/*------------------------------------------------------------------------------
 *  File identity
 *----------------------------------------------------------------------------*/
static const char fileid[] = "$Id$";


/* ===============================================  local function prototypes */


/* =============================================================  module code */

/*------------------------------------------------------------------------------
 *  Initialize the object
 *----------------------------------------------------------------------------*/
void
DarkIce :: init ( void )                            throw ( Exception )
{
    /* the pipes */
    encOutPipe      = new PipeSource( "enc.out");
    encInPipe       = new PipeSink( "enc.in");
    
    if ( !encOutPipe->exists() ) {
        if ( !encOutPipe->create() ) {
            throw Exception( __FILE__, __LINE__, "can't create out pipe");
        }
    }

    if ( !encInPipe->exists() ) {
        if ( !encInPipe->create() ) {
            throw Exception( __FILE__, __LINE__, "can't create in pipe");
        }
    }

    /* encoder related stuff */
    dsp             = new OssDspSource( "/dev/dsp", 22050, 16, 2);
    encIn           = new BufferedSink( encInPipe.get(), 64 * 1024);
    encConnector    = new Connector( dsp.get(), encInPipe.get());
    encoder         = new LameEncoder( "notlame",
                                        encInPipe->getFileName(),
                                        dsp.get(),
                                        encOutPipe->getFileName(),
                                        96 );


    /* streaming related stuff */
    socket          = new TcpSocket( "susi", 8000);
    ice             = new IceCast( socket.get(),
                                   "hackme",
                                   "sample",
                                   "name",
                                   "description",
                                   "http://ez.az/",
                                   "sajat",
                                   128,
                                   false );
    shoutConnector  = new Connector( encOutPipe.get(), ice.get());
}


/*------------------------------------------------------------------------------
 *  Run the encoder
 *----------------------------------------------------------------------------*/
bool
DarkIce :: encode ( void )                          throw ( Exception )
{
    unsigned int       len;

    encoder->start();

    sleep( 1 );

    if ( !encConnector->open() ) {
        throw Exception( __FILE__, __LINE__, "can't open connector");
    }
    
    len = encConnector->transfer( 22050 * 2 * 2 * 120, 4096, 1, 0 );

    cout << len << " bytes transfered" << endl;

    encConnector->close();

    encoder->stop();

    return true;
}


/*------------------------------------------------------------------------------
 *  Run the encoder
 *----------------------------------------------------------------------------*/
bool
DarkIce :: shout ( void )                           throw ( Exception )
{
    unsigned int       len;

    if ( !shoutConnector->open() ) {
        throw Exception( __FILE__, __LINE__, "can't open connector");
    }
    
    len = shoutConnector->transfer( 128 * 1024 / 8 * 120, 4096, 1, 0 );

    cout << len << " bytes transfered" << endl;

    shoutConnector->close();

    return true;
}


/*------------------------------------------------------------------------------
 *  Run
 *----------------------------------------------------------------------------*/
int
DarkIce :: run ( void )                             throw ( Exception )
{
    pid_t   pid;
    
    cout << "DarkIce" << endl << endl << flush;

    init();

    cout << "init OK" << endl << flush;

    pid = fork();

    if ( pid == -1 ) {
        throw Exception( __FILE__, __LINE__, "fork error", errno);
        
    } else if ( pid == 0 ) {
        // this is the child

        sleep ( 2 );

        cout << "shouting" << endl << flush;
        shout();
        cout << "shouting ends" << endl << flush;

        exit(0);
    } else {
        // this is the parent
        int     status;

        cout << "encoding" << endl << flush;
        encode();
        cout << "encoding ends" << endl << flush;

        waitpid( pid, &status, 0);
        if ( !WIFEXITED(status) ) {
            throw Exception( __FILE__, __LINE__,
                             "child exited abnormally", WEXITSTATUS(status));
        }
    }

    return 0;
}


/*------------------------------------------------------------------------------
 
  $Source$

  $Log$
  Revision 1.1  2000/11/05 10:05:49  darkeye
  Initial revision

  
------------------------------------------------------------------------------*/


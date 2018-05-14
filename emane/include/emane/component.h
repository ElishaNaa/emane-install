/*
 * Copyright (c) 2013-2014 - Adjacent Link LLC, Bridgewater, New Jersey
 * Copyright (c) 2008 - DRS CenGen, LLC, Columbia, Maryland
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of DRS CenGen, LLC nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef EMANECOMPONENT_HEADER_
#define EMANECOMPONENT_HEADER_

#include "emane/registrar.h"
#include "emane/configurationupdate.h"

#include <list>

namespace EMANE
{
  /**
   * @class InitializeException
   *
   * @brief  Exception thrown during component initialization
   */
  class InitializeException{};


  /**
   * @class StopException
   *
   * @brief  Exception thrown during a component stop
   */
  class StopException{};
  

  /**
   * @class Component
   *
   * @brief Generic interface used to configure and control all components.
   *
   * @details The component interface is used to transition all components
   * through the component state machine.
   *
   * @dot
   * digraph G {
   *  node [style="rounded,filled", fillcolor="yellow",shape="box"] 
   *  ComponentUninitialized -> ComponentInitialized [label=" initialize "]
   *  ComponentInitialized -> ComponentConfigured [label=" configure "]
   *  ComponentConfigured -> ComponentRunning [label=" start "]
   *  ComponentRunning -> ComponentStopped [label=" stop "]
   *  ComponentStopped -> ComponentRunning [label=" start "]
   *  ComponentRunning -> ComponentRunning [label=" postStart "]
   *  ComponentStopped -> ComponentDestroyed [label=" destroy "]
   * }
   * @enddot
   */
  class Component
  {
  public:
    virtual ~Component(){}
   
    /**
     * Initialize the component.
     *
     * @exception InitializeException thrown when an error is encountered during
     * initialization
     */
    virtual void initialize(Registrar & registrar) = 0;
    
    /**
     * Configure the component.
     *
     * @param update Configuration update values
     *
     * @exception ConfigureException thrown when a unexpected configuration item is
     * encountered or there is a problem with the specified item value
     */
    virtual void configure(const ConfigurationUpdate & update) = 0;

    /**
     * Start the component.
     *
     * @exception StartException thrown when an error is encountered during
     * start.
     *
     */
    // start the component
    virtual void start() = 0;


    /**
     * Hook to run any post start functionaililty.  Called after all the
     * components have been started.
     */
    virtual void postStart(){};

    /**
     * Stop the component.
     *
     * @exception StopException thrown when an error is encountered during
     * stop
     */
    virtual void stop() = 0;
    
    /**
     * Destroy the component.
     */
    virtual void destroy() throw() = 0;
    
  protected:
    Component(){}
  };
}

#endif //EMANECOMPONENT_HEADER_


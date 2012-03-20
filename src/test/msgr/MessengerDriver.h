/*
 * Ceph - scalable distributed file system
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 */

#ifndef MESSENGERDRIVER_H_
#define MESSENGERDRIVER_H_

#include "msg/Messenger.h"
#include "TestDriver.h"

#include <boost/scoped_ptr.hpp>

/**
 * The MessengerDriver is a fairly simple object which takes responsibility
 * for a single Messenger, does its generic setup, translates requested
 * actions from the TestDriver into specific Messenger commands, and relays
 * important event changes from the Messenger to the TestDriver.
 */
class MessengerDriver : public Dispatcher {
  /**
   * The TestDriver which I report to.
   */
  TestDriver *driver;

  /**
   * The actual Messenger this MessengerDriver operates on.
   */
  boost::scoped_ptr<Messenger> messenger;

public:
  CephContext *cct;
  /**
   * Construct a MessengerDriver.
   *
   * @param testdriver: The TestDriver object this MessengerDriver
   * will send reports back to.
   * @param msgr: The Messenger this MessengerDriver will handle. msgr
   * must be a valid Messenger, and any implementation-specific options must
   * be set, but the MessengerDriver will perform all of generic Messenger
   * startup and takes over the reference to it.
   */
  MessengerDriver(TestDriver *testdriver, Messenger *msgr) :
    Dispatcher(msgr->cct), driver(testdriver), messenger(msgr), state(BUILT) {}

  virtual ~MessengerDriver() { assert(state == STOPPED || state == FAILED); }

  /**
   * Initialize the MessengerDriver and its components. Call this function
   * before doing anything else after construction.
   * Once this function completes, the MessengerDriver and its Messenger
   * are ready to go and have started running.
   *
   * @return 0 on success, -errno on failure.
   */
  int init();

  /**
   * Stop the Messenger and shut everything down. This should be called once
   * before destruction, and nothing else should be called afterwards.
   *
   * @return -1 if this is currently invalid, -errno on a failure, 0 otherwise.
   */
  int stop();

  /**
   * @defgroup Accessors
   * @{
   */
  /**
   * Get the address of the Messenger we control.
   *
   * @return A constant reference to the address.
   */
  const entity_addr_t& get_addr() { return messenger->get_myaddr(); }
  const entity_inst_t& get_inst() { return messenger->get_myinst(); }
  /**
   * @} Accessors
   */


  /**
   * @defgroup Orders
   * @{
   */
  /**
   * Send a message to the given entity. Completion of this function
   * does not guarantee delivery, but it does guarantee you get a notification
   * of either message send or connection break. (There may be other guarantees
   * depending on the Policy that applies between this entity and the recipient.)
   *
   * @param message The message to send. We take control
   * of the reference we are passed.
   * @param dest The entity to send the message to.
   *
   * @return -1 if this command is invalid (Messenger is shut down or
   * unitialized), 0 otherwise.
   */
  int send_message(Message *message, const entity_inst_t& dest);

  /**
   * Establish a connection to the given endpoint. This is not synchronous -- it
   * initiates the connection but more work may be required for it to finish. If
   * we already have an active connection, this function is currently a no-op.
   * LATER: make it tell you if the connection already exists.
   *
   * @param dest The entity to connect to.
   *
   * @return -1 if this command is invalid (Messenger is shut down or
   * unitialized), 0 otherwise.
   */
  int establish_connection(const entity_inst_t& dest);

  /**
   * Break any connection to the given entity. This is currently equivalent
   * to calling mark_down().
   * If we do not have an active connection with the given endpoint, this
   * function is a no-op.
   * LATER: make it tell you if there's no connection.
   *
   * @param other The entity to break your connection with.
   *
   * @return -ENOTCONN if there is not a connection to other, -1 if this
   * command is invalid (Messenger is shut down or uninitialized), 0 otherwise.
   */
  virtual int break_connection(const entity_inst_t& other);
  /**
   * @} Orders
   */

  /**
   * @defgroup Dispatcher
   * @{
   */
  virtual bool ms_dispatch(Message *m);
  /**
   * @} Dispatcher
   */

protected:
  enum STATE { BUILT, RUNNING, STOPPED, FAILED };
  STATE state;
};

#endif /* MESSENGERDRIVER_H_ */

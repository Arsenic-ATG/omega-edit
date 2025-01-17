/*
 * Copyright 2021 Concurrent Technologies Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.ctc.omega_edit.grpc

import akka.NotUsed
import akka.actor.{Actor, ActorLogging, Props}
import akka.stream.scaladsl.Source
import com.ctc.omega_edit.api
import com.ctc.omega_edit.api.ViewportCallback
import com.ctc.omega_edit.grpc.Editors.{BooleanResult, Ok, ViewportData}
import com.ctc.omega_edit.grpc.Viewport.{
  Destroy,
  EventStream,
  Events,
  Get,
  HasChanges,
  Modify,
  Unwatch,
  Watch
}
import com.google.protobuf.ByteString
import omega_edit.ObjectId

import java.util.UUID
import omega_edit.ViewportEvent

import scala.annotation.unused

object Viewport {
  type EventStream = Source[ViewportEvent, NotUsed]
  trait Events {
    def stream: EventStream
  }

  def props(
      view: api.Viewport,
      stream: EventStream,
      cb: ViewportCallback
  ): Props =
    Props(new Viewport(view, stream, cb))

  case class Id(session: String, view: String)
  object Id {
    def unapply(oid: ObjectId): Option[(String, String)] =
      oid.id.split(":") match {
        case Array(s, v) => Some((s, v))
        case _           => None
      }

    def uuid(): String = UUID.randomUUID().toString
  }

  trait Op
  case class Modify(offset: Long, capacity: Long, isFloating: Boolean)
      extends Op
  case object Get extends Op
  case object HasChanges extends Op
  case object Destroy extends Op
  case class Watch(eventInterest: Option[Int]) extends Op
  case object Unwatch extends Op

}

class Viewport(
    view: api.Viewport,
    events: EventStream,
    @unused cb: ViewportCallback // need to keep a reference to the callback to prevent it from being GC'd
) extends Actor
    with ActorLogging {
  val viewportId: String = self.path.name

  private def generateViewportData(
      viewport: api.Viewport,
      id: String
  ): Ok with ViewportData = new Ok(id) with ViewportData {
    def data: ByteString = ByteString.copyFrom(viewport.data)
    def offset: Long = viewport.offset
    def followingByteCount: Long = viewport.followingByteCount
  }

  def receive: Receive = {

    case Modify(offset, capacity, isFloating) =>
      view.modify(offset, capacity, isFloating)
      sender() ! generateViewportData(view, viewportId)

    case Get =>
      sender() ! generateViewportData(view, viewportId)

    case HasChanges =>
      sender() ! new Ok(viewportId) with BooleanResult {
        def result: Boolean = view.hasChanges
      }

    case Destroy =>
      view.destroy()
      sender() ! Ok(viewportId)

    case Watch(eventInterest) =>
      view.eventInterest =
        eventInterest.getOrElse(api.ViewportEvent.Interest.All)
      sender() ! new Ok(viewportId) with Events {
        def stream: EventStream = events
      }

    case Unwatch =>
      view.eventInterest = 0
      sender() ! Ok(viewportId)
  }
}

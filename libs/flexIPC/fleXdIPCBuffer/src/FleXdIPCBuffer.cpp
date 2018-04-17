/*
Copyright (c) 2017, Globallogic s.r.o.
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
 * Neither the name of the Globallogic s.r.o. nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL GLOBALLOGIC S.R.O. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* 
 * File:   FleXdIPCBuffer.cpp
 * Author: Adrian Peniak
 * Author: Matus Bodorik
 * 
 * Created on January 31, 2018, 8:32 PM
 */

#include "FleXdLogger.h"
#include "FleXdIPCBuffer.h"
#include "FleXdIPCFactory.h"
#include <iostream>
#include <unistd.h>
#include <vector>

namespace flexd {
    namespace ilc {
        namespace epoll {
            

            FleXdIPCBuffer::FleXdIPCBuffer(size_t maxBufferSize)
            : m_maxBufferSize(maxBufferSize),
              m_queue(),
              m_onMsg(nullptr), 
              m_factory(new FleXdIPCFactory([this](pSharedFleXdIPCMsg msg){releaseMsg(msg);}))
            {
                  FLEX_LOG_TRACE("FleXdIPCBuffer -> Start");
            }

            FleXdIPCBuffer::FleXdIPCBuffer(std::function<void(pSharedFleXdIPCMsg msg)> onMsg, size_t maxBufferSize)
            : m_maxBufferSize(maxBufferSize),
              m_queue(),
              m_onMsg(onMsg),
              m_factory(new FleXdIPCFactory([this](pSharedFleXdIPCMsg msg){releaseMsg(msg);}))
            {
                  FLEX_LOG_TRACE("FleXdIPCBuffer -> Start");
            }
            
            FleXdIPCBuffer::~FleXdIPCBuffer()
	    {
                FLEX_LOG_TRACE("FleXdIPCBuffer -> Destroyed");
	    }
            

            FleXdIPCBuffer::FleXdIPCBuffer(FleXdIPCBuffer&& other)
            : m_maxBufferSize(other.m_maxBufferSize),
              m_queue(std::move(other.m_queue)),
              m_onMsg(other.m_onMsg),
              m_factory(std::move(other.m_factory))            
            {
                FLEX_LOG_TRACE("FleXdIPCBuffer moved");
            }
            
            FleXdIPCBuffer& FleXdIPCBuffer::operator=(FleXdIPCBuffer&& other)
            {
                FLEX_LOG_TRACE("FleXdIPCBuffer moved");
                m_maxBufferSize = other.m_maxBufferSize;
                m_queue = std::move(other.m_queue);
                m_onMsg = other.m_onMsg;
                m_factory = std::move(other.m_factory);
                return *this;
            }
                        
            void FleXdIPCBuffer::rcvMsg(pSharedArray8192& data, size_t size) {
                if(m_factory) {
                FLEX_LOG_TRACE("FleXdIPCBuffer::rcvMsg() -> Send data to Factory");
                    m_factory->parseData(data,size);
		} else {
                    FLEX_LOG_ERROR("FleXdIPCBuffer::rcvMsg() -> Error: Factory does not exist!");
                }
            }
	    
            pSharedFleXdIPCMsg FleXdIPCBuffer::front() const {
                return m_queue.front();
            }

            pSharedFleXdIPCMsg FleXdIPCBuffer::back() const {
                return m_queue.back();
            }

            pSharedFleXdIPCMsg FleXdIPCBuffer::pop() {
                auto ret = m_queue.front();
                m_queue.pop();
                return std::move(ret);
            }
            
            void FleXdIPCBuffer::releaseMsg(pSharedFleXdIPCMsg msg)
            {
                if(msg->isComplete())
                {
                    if(m_onMsg)
                    {
                        m_onMsg(std::move(msg));
                    } else {
                        if(m_bufferSize < m_maxBufferSize)
                        {
                            m_queue.push(std::move(msg));
                            m_bufferSize += msg->getMsgSize();
                        } else {
                            FLEX_LOG_WARN("FleXdIPCBuffer::releaseMsg() -> onMsg = NULL and buffer is full");
                        }       
                    }
                } else {
                    if(m_onMsg)
                    {
                        m_onMsg(nullptr);
                    } else {
                        FLEX_LOG_ERROR("FleXdIPCBuffer::releaseMsg() -> onMsg = NULL!");
                    }
                }
            }
        } // namespace epoll
    } // namespace ilc
} // namespace flexd
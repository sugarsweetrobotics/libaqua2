#pragma once

/**********************************************************
 * gamepad.h
 *
 *
 * Portable Class Library for Windows and Unix.
 * @author ysuga@ysuga.net
 * @date 2019/04/12
 ********************************************************/
#pragma once
#include <iostream>
#include <vector>
#include <exception>


#ifdef WIN32
#pragma comment(lib, "winmm.lib")
#include <windows.h>

#elif LINUX

#include <iomanip>
#include <vector>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

#elif __APPLE__
#include <IOKit/hid/IOHIDManager.h>
#include <thread>
#endif

extern "C" {

static void device_input(void* ctx, IOReturn result, void* sender, IOHIDValueRef value);

static void append_matching_dictionary(CFMutableArrayRef matcher, uint32_t page, uint32_t use) {
    CFMutableDictionaryRef result = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (!result) return;
    CFNumberRef pageNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &page);
    CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsagePageKey), pageNumber);
    CFRelease(pageNumber);
    CFNumberRef useNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &use);
    CFDictionarySetValue(result, CFSTR(kIOHIDDeviceUsageKey), useNumber);
    CFRelease(useNumber);
    CFArrayAppendValue(matcher, result);
    CFRelease(result);
}

static void device_attached(void* ctx, IOReturn result, void* sender, IOHIDDeviceRef device);

static void device_detached(void* ctx, IOReturn result, void* sender, IOHIDDeviceRef device);
}

namespace ssr { 
  namespace aqua2 {

  class GamePad;
  class DeviceNotFoundException : public std::exception {
  };

  class GamePad {
  public:
    std::vector<float> axis;
    std::vector<bool> buttons;
    std::vector<bool> old_buttons;


  private:
#ifdef LINUX
    int joy_fd;
    int num_of_axis;
    int num_of_buttons(0);
    std::vector<char> joy_button;
    std::vector<int> joy_axis;
    char name_of_joystick[80];
#elif WIN32
    JOYINFOEX joyInfo_;
    int num_of_axis;
    int num_of_buttons;
#elif __APPLE__
    
    IOHIDManagerRef ioHIDManagerRef_;
    std::vector<std::pair<std::string,IOHIDDeviceRef>> devices_;
    std::thread* thread_;
    std::vector<float> axis_buf_;
    std::vector<bool> buttons_buf_;
#endif
  public:

    GamePad(const char* filename) {
#if LINUX
      if((joy_fd=open(filename, O_RDONLY)) < 0) { throw new JoystickNotFoundException();}
  
      ioctl(joy_fd, JSIOCGAXES, &num_of_axis);
      ioctl(joy_fd, JSIOCGBUTTONS, &num_of_buttons);
      ioctl(joy_fd, JSIOCGNAME(80), &name_of_joystick);
      
      buttons.resize(num_of_buttons,0);
      old_buttons.resize(num_of_buttons,0);
      axis.resize(num_of_axis,0);
  
      fcntl(joy_fd, F_SETFL, O_NONBLOCK);   // using non-blocking mode

#elif WIN32
 
      joyInfo_.dwSize = sizeof(JOYINFOEX);
      joyInfo_.dwFlags = JOY_RETURNALL;

      num_of_buttons = 16;
      num_of_axis = 7;
      buttons.resize(num_of_buttons, 0);
      old_buttons.resize(num_of_buttons, 0);
      axis.resize(num_of_axis, 0);
#elif __APPLE__

      CFMutableArrayRef matcher;
      if (!(ioHIDManagerRef_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone))) {
          return;
      }
      if (!(matcher = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks))) {
          IOHIDManagerUnscheduleFromRunLoop(ioHIDManagerRef_, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
          IOHIDManagerClose(ioHIDManagerRef_, kIOHIDOptionsTypeNone);
          CFRelease(ioHIDManagerRef_);
          return;
      }
      
      append_matching_dictionary(matcher, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick);
      append_matching_dictionary(matcher, kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad);

      //append_matching_dictionary(matcher, kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
      IOHIDManagerSetDeviceMatchingMultiple(ioHIDManagerRef_, matcher);
      CFRelease(matcher);
      
      IOHIDManagerRegisterDeviceMatchingCallback(ioHIDManagerRef_, device_attached, (void*)this);
      IOHIDManagerRegisterDeviceRemovalCallback(ioHIDManagerRef_, device_detached, this);


      thread_ = new std::thread([this]() {
        IOHIDManagerScheduleWithRunLoop(ioHIDManagerRef_, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
        IOHIDManagerOpen(ioHIDManagerRef_, kIOHIDOptionsTypeNone);
        CFRunLoopRun();
      });


    int num_of_axis;
    int num_of_buttons;
      num_of_buttons = 16;
      num_of_axis = 7;
      axis.resize(num_of_axis,0);
      axis_buf_.resize(num_of_axis,0);
      buttons.resize(num_of_buttons, 0);
      buttons_buf_.resize(num_of_buttons, 0);
      old_buttons.resize(num_of_buttons, 0);
#endif
    }


    ~GamePad() {


#ifdef __APPLE__
        thread_->detach();
        delete thread_;
        IOHIDManagerUnscheduleFromRunLoop(ioHIDManagerRef_, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
        IOHIDManagerClose(ioHIDManagerRef_, kIOHIDOptionsTypeNone);
        CFRelease(ioHIDManagerRef_);
#endif
    }
  public:
    void update() {
#ifdef WIN32
      if(JOYERR_NOERROR != joyGetPosEx(0, &joyInfo_)) {
        return;
      }
      for (size_t i = 0; i < buttons.size(); i++) {
          old_buttons[i] = buttons[i];
          if (joyInfo_.dwButtons & (0x00000001 << i)) {
              buttons[i] = 1;
          }
          else {
              buttons[i] = 0;
          }
      }

      axis[0] = ((float)joyInfo_.dwXpos - 32767.0f) / 32767.0f;
      axis[1] = ((float)joyInfo_.dwYpos - 32767.0f) / 32767.0f;
      axis[3] = ((float)joyInfo_.dwZpos - 32767.0f) / 32767.0f;
      axis[2] = ((float)joyInfo_.dwUpos - 32767.0f) / 32767.0f;
      axis[4] = ((float)joyInfo_.dwRpos - 32767.0f) / 32767.0f;

      switch (joyInfo_.dwPOV) {
      case 0:
          axis[5] = 0.0f;
          axis[6] = -1.0f;
          break;
      case 9000:
          axis[5] = +1.0f;
          axis[6] = 0.0f;
          break;
      case 18000:
          axis[5] = 0.0f;
          axis[6] = +1.0f;
          break;
      case 27000:
          axis[5] = -1.0f;
          axis[6] = 0.0f;
          break;
      case 4500:
          axis[5] = +1.0f;
          axis[6] = -1.0f;
          break;
      case 13500:
          axis[5] = +1.0f;
          axis[6] = +1.0f;
          break;
      case 22500:
          axis[5] = -1.0f;
          axis[6] = +1.0f;
          break;
      case 31500:
          axis[5] = -1.0f;
          axis[6] = -1.0f;
          break;
      default:
          axis[5] = 0.0f;
          axis[6] = 0.0f;
          break;
      }
#elif LINUX
      js_event js;
      read(joy_fd, &js, sizeof(js_event));
      switch (js.type & ~JS_EVENT_INIT) {
      case JS_EVENT_AXIS:
	if((int)js.number>=joy_axis.size())  {std::cerr<<"err:"<<(int)js.number<<std::endl;}
	axis[(int)js.number]= js.value / 300.0;
	break;
      case JS_EVENT_BUTTON:
	if((int)js.number>=joy_button.size())  {std::cerr<<"err:"<<(int)js.number<<std::endl;}
	button_old[(int)js.number] = button[(int)js.number];
	button[(int)js.number]= js.value;
	break;
      }
#elif __APPLE__
    axis = axis_buf_;
    old_buttons = buttons;
    buttons = buttons_buf_;
#endif
    }


#ifdef __APPLE__
    void attachDevice(CFStringRef name, IOHIDDeviceRef device) {
        char name_buf[1024];
        CFStringGetCString(name, name_buf, 1024, kCFStringEncodingUTF8);
        devices_.push_back({std::string(name_buf), device});

        IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);
        IOHIDDeviceScheduleWithRunLoop(device, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
        IOHIDDeviceRegisterInputValueCallback(device, device_input, this);
    }

    void detachDevice(IOHIDDeviceRef device) {
        auto it = devices_.begin();
        for(;it != devices_.end();) {
            if ((*it).second == device) {
                IOHIDDeviceClose(device, kIOHIDOptionsTypeNone);
                it = devices_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void inputDevice(int a, int b, int c, double d) {
        switch(c) {
        case 48:
            axis_buf_[0] = (double)d / 128.0  - 1.0;
            return;
        case 49:
            axis_buf_[1] = (double)d*2 / 254.0 - 1.0;
            return;
        case 50:
            axis_buf_[3] = (double)d*2 / 256.0 - 1.0;
            return;
        case 53:
            axis_buf_[4] = (double)d*2 / 254.0 - 1.0;
            return;
        }

        if (c == 57) {
            switch ((int)d) {
            case 0:
                axis_buf_[5] = 0.0f;
                axis_buf_[6] = -1.0f;
                break;
            case 90:
                axis_buf_[5] = +1.0f;
                axis_buf_[6] = 0.0f;
                break;
            case 180:
                axis_buf_[5] = 0.0f;
                axis_buf_[6] = +1.0f;
                break;
            case 270:
                axis_buf_[5] = -1.0f;
                axis_buf_[6] = 0.0f;
                break;
            case 45:
                axis_buf_[5] = +1.0f;
                axis_buf_[6] = -1.0f;
                break;
            case 135:
                axis_buf_[5] = +1.0f;
                axis_buf_[6] = +1.0f;
                break;
            case 225:
                axis_buf_[5] = -1.0f;
                axis_buf_[6] = +1.0f;
                break;
            case 315:
                axis_buf_[5] = -1.0f;
                axis_buf_[6] = -1.0f;
                break;
            default:
                axis_buf_[5] = 0.0f;
                axis_buf_[6] = 0.0f;
                break;
            }
            return;
        }

        if(a == 2) {
            buttons_buf_[c] = (int)d;
            return;
        }

        //if (b != 65280)
        // std::cout << "inputDevice(" << a << ", " << b << ", " << c << ", " << d << ")" << std::endl;
    }


#endif

  };

#ifdef __APPLE__

#endif






  }
  
}

extern "C" {
static void device_input(void* ctx, IOReturn result, void* sender, IOHIDValueRef value)
{
    IOHIDElementRef element = IOHIDValueGetElement(value);
    if (ctx) static_cast<ssr::aqua2::GamePad*>(ctx)->inputDevice((int)IOHIDElementGetType(element), (int)IOHIDElementGetUsagePage(element), (int)IOHIDElementGetUsage(element), 
    IOHIDValueGetScaledValue(value, kIOHIDValueScaleTypePhysical));//(int)IOHIDValueGetIntegerValue(value));
}

void device_attached(void* ctx, IOReturn result, void* sender, IOHIDDeviceRef device) {
   if (ctx) static_cast<ssr::aqua2::GamePad*>(ctx)->attachDevice(static_cast<CFStringRef>(IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey))), device);
}


static void device_detached(void* ctx, IOReturn result, void* sender, IOHIDDeviceRef device) {
   if (ctx) static_cast<ssr::aqua2::GamePad*>(ctx)->attachDevice(static_cast<CFStringRef>(IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey))), device);

}

}
# PRODUCT OVERVIEW

RemoteBridge is a library that allows you to access internal COM and JAVA
objects used by third-party applications remotely. This product was designed to
support new and legacy products were source code is unavailable or modifications
costs are expensive.

Once you get access to an object using the RemoteBridge interface, you can
interact with it in any way, for e.g., setting/getting properties, calling
methods, etc., like if you were running custom injected code inside the target
application.

Most of times, objects managed by an application are not accessible from outside.
Also, although you can inject and hook custom code in an application, those kind
of objects cannot be easily reached.

RemoteBridge hooks the target application and inspects all COM and JAVA
activity. You will be able to see all COM objects like if they were local to
your application. It also creates a wrapper interface for all JAVA objects.

RemoteBridge uses a COM interface supported by most programming languages such
as C++, Delphi, VB, VBA, VB.SCRIPT, VB.NET, C# and Python. COM objects used by
the third-party application will be directly available as COM-proxy objects in
your code. For Java ones, an interface named INktJavaObject is provided as a
wrapper.

The examples provided will show the power of this library. For e.g.,

- Inspect the new IFileOpenDialog COM object (introduced in Windows Vista) used
  in the classic File Open dialog box and retrieve the current typed file name.
- Retrieve the internal IHTMLDocument2 COM object in Internet Explorer, add
  a custom OnDocumentComplete event handler and modify page HTML code.
- Add custom panels and change object properties in the Java Notepad application
  provided in the Java Development Kit.
- Inject custom Java code into a target application and interact with it by
  executing methods and listening to events.


----------------------
# MINIMUM REQUIREMENTS

To use RemoteBridge you must have the following:

- IBM PC or compatible.
- Microsoft Windows XP or later.
- Visual Studio 2012 with Update 4 (or newer) installed.


----------------------
# INSTALLATION & USAGE

Download RemoteBridge from GitHub by executing the following Git command:

    git clone --recursive https://github.com/nektra/RemoteBridge.git

Instead, if you decide to downloaded the .zip file from GitHub , uncompress it
in an empty folder and then download and uncompress the Deviare In-Proc
dependency (from https://github.com/nektra/Deviare-InProc) inside the following
subdirectory: "Dependencies\Deviare-InProc".

To use the library in your project, add the reference to the COM object located
in RemoteBridge.dll or RemoteBridge64.dll depending on the target platform.

A C# assembly is also provided. If you use RemoteBridge in C# we strongly
recommend to add the Nektra.RemoteBridge.dll reference instead.

RemoteBridge supports RegFree-COM model by adding the corresponding manifest
file to your application. If you want to use the standard COM mechanism,
remember to register RemoteBridge.dll and/or RemoteBridge64.dll using the
RegSvr32 utility located in the "%sysroot%\system32" folder.


----------------------
# NOTES ON COM HOOKING

1. The following interfaces are not hookable. They are mainly used by OLE32
   internally and most of them are not available to the end user:

   IID_IGlobalInterfaceTable, IID_IRunningObjectTable, IID_IAggregator,
   IID_IMarshal, IID_IStdMarshalInfo, IID_IdentityUnmarshal, IID_IRemUnknown,
   IID_IRundown, IID_IPSFactory, IID_IPSFactoryBuffer, IID_IMultiQI,
   IID_IProxy, IID_IRpcChannel, IID_IRpcHelper, IID_IRpcOptions, IID_IRpcStub,
   IID_IRpcStubBuffer, IID_IRpcProxy, IID_IRpcProxyBuffer, IID_IStubManager,
   IID_IProxyManager, IID_IRpcChannelBuffer, IID_IAsyncRpcChannelBuffer,
   IID_IRpcSyntaxNegotiate, IID_IInternalMoniker, IID_IChannelHook,
   IID_IClientSecurity, IID_IServerSecurity, IID_ICallFactory,
   IID_IReleaseMarshalBuffers, IID_IComThreadingInfo, IID_IContext &
   IID_IObjContext.

2. Some interfaces requires to be registered in order to work (specially those
   using a non-standard marshalling mechanism). If you can't access an object,
   check if it is registered in Windows' Registry.

3. Some interfaces are not instantiated as expected. For e.g., if you hook the
   IHTMLDocument interface that belongs to the CLSID_HtmlDocument object, you
   may encounter that the object is created using the IUnknown interface
   instead. Use the 'flgDebugPrintInterfaces' flag to get information about
   how different interfaces are instantiated.


-------------
# BUG REPORTS

If you experience something you think might be a bug in RemoteBridge, please
report it in GitHub repository or write us in <http://www.nektra.com/contact/>.

Describe what you did, what happened, what kind of computer you have,
which operating system you're using, and anything else you think might
be relevant.


-----------------------
# LICENSING INFORMATION

This library has a dual license, a commercial one suitable for closed source
projects and a GPL license that can be used in open source software.

Depending on your needs, you must choose one of them and follow its policies.
A detail of the policies and agreements for each license type are available in
the *LICENSE.COMMERCIAL* and *LICENSE.GPL* files.

For further information please refer to <http://www.nektra.com/licensing/> or
contact Nektra here <http://www.nektra.com/contact/>.

This library uses a portion of UDis86 project <http://udis86.sourceforge.net/>,
authored, copyrighted and maintained by Vivek Thampi. UDis86 is licensed under
the terms of BSD License. For any questions referring to UDis86 contact the
author at <vivek[at]sig9[dot]com>.

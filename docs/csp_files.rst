Working with CSP+ files
=======================
CSP+ files are used to describe slave stations. By loading them into the
engineering tool, it is easy to setup the communication between the PLC and
CCIEFB slave stations.

The files are in XML format. Due to the complexity of the specific XML format,
it is recommended to use the CSP+ tool provided by the CC-Link organisation.

Follow the document "CSP+ creation guidelines BAP-C3001ENG-001" for a description
of how to use the CSP+ tool.
The CSP+ version 2.2 specification is found in the BAP-C2008ENG-001 document.

An example CSP+ file is available in the :file:`sample_apps/` directory
in the c-link repository.


Installing the CSP+ tool on Windows
-----------------------------------
#. Download the "CSP+ profile creation support tool" from the CC-Link
   homepage. It is important that you get the right version of the tool.
   At least version 4.0 is required to work with CCIEFB. You need to be logged into the
   CC-link homepage to be able to download it.

#. Unzip the downloaded file, no formal installation is required.

#. Double-click the "ProfileCreationTool" file. The first time, you need to set the
   target, via a popup window. Select "CSP+[2.2]" and "Ethernet/CC-Link IEF
   Basic [2.1]".


Using the CSP+ tool
--------------------
#. Create a new document by using the menu :menuselection:`File --> New`.

   These are the sections of a CSP+ file:

   * The "File" section describes the file itself (creation date etc).
   * The "Device" section describes the overall device features, like its
     ProductID and its physical height and weight.
   * The input and output data is described in the "Block" section.
   * Each available communication protocol has its own "Commif" section.

#. In the initial pop-up window, enter these values (leave others empty):

   =================================== ===============
   Entry                               Value
   =================================== ===============
   Label of file section               FileSection
   Label of file_info part             FileInfo
   Label of device section             DeviceSection
   Label of of device_info part        DeviceInfo
   =================================== ===============

   The contents of the FILE_INFO part is typically generated automatically,
   and the dates are automatically updated when exporting the CSP+ file.

Device information
``````````````````

#. Locate the DEVICE_INFO node in the tree (DEVICE > DeviceSection > DeviceInfo),
   and double-click it to open the editor view in the right half of the application.

#. In the DATA column, enter these values:

   ====================== ================= ==================================================================
   Row ("LABEL")          Value             Note
   ====================== ================= ==================================================================
   VendorName             RT-Labs           Replace with your organisation's name.
   VendorCode             0x1067            Replace with your organisation's value. Use hexadecimal number.
   DeviceModel            Sampleapp         Will end up in the file name, so avoid space characters.
   ProductID              0x87654321        Optional. Use hexadecimal number.
   DeviceTypeId           0x03              0x03 = digital I/O. Can be replaced by DeviceTypeDetail. Use hex.
   Version                0x0002            Use hexadecimal number.
   VersionDisplayFlg      1                 1 = show version to user
   VersionPolicyType      0                 0 = no need for awareness
   ====================== ================= ==================================================================

   The DeviceTypeId values are listed in the document "list_of_modeltype.pdf".

#. Remove the remaining lines by right-clicking and select :guilabel:`Delete Element`.


Cyclic data
```````````
In this example we will use this data:

=========== =============================== ========= ===================== ============== ==============
LABEL       NAME                            DATATYPE  Direction             COMM_IF        BLOCK
=========== =============================== ========= ===================== ============== ==============
RX0         Button                          BOOL      Slave -> master (PLC) COMM_IF_INPUT  BLOCK_OUTPUT
RWr0        Button press counter            UINT16    Slave -> master (PLC) COMM_IF_INPUT  BLOCK_OUTPUT
RY16 (dec)  LED                             BOOL      Master (PLC) -> slave COMM_IF_OUTPUT BLOCK_INPUT
RWw0        Arbitrary integer from the PLC  UINT16    Master (PLC) -> slave COMM_IF_OUTPUT BLOCK_INPUT
=========== =============================== ========= ===================== ============== ==============

Note that RX and RWr (which are sent from slave to master/PLC) are categorised
as inputs from COMM_IF point of view, but outputs from BLOCK point of view.

Cyclic data registers and bits are entered as hexadecimal numbers in the tool,
so use ``RY10`` for RY16 (dec).

#. In the tree-view, right-click the BLOCK folder icon, and use :guilabel:`Add Section`.

#. In the pop-up window, select BLOCK section type. Enter "BlockSection" for the
   label of the section and "BlockInfo" for the label of the BLOCK_INFO.
   Leave the other fields empty.

#. In the BlockInfo, update the values for VendorName, VendorCode and Version.
   Use the same values as in the DeviceInfo.

#. Right-click BlockSection and add a part.

#. In the pop-up window, select BLOCK_INPUT with label "BlockInput" and 2 elements.

#. Open the BlockInput editor by double-clicking.

#. Enter these values:

   ======= =============================== =========
   LABEL   NAME                            DATATYPE
   ======= =============================== =========
   RY10    LED                             BOOL
   RWw0    Arbitrary integer from the PLC  UINT16
   ======= =============================== =========

   Note that BLOCK_INPUT describes data that is input to the slave (from the PLC).

#. Similarly, add a BLOCK_OUTPUT with label "BlockOutput" and 2 elements.
   Use these values:

   ======= ======================= =========
   LABEL   NAME                    DATATYPE
   ======= ======================= =========
   RX0     Button                  BOOL
   RWr0    Button press counter    UINT16
   ======= ======================= =========


Communication settings
``````````````````````
#. Add a COMM_IF section by right-clicking the COMM_IF icon in the tree, and
   use :guilabel:`Add Section`.

#. Enter "CommIf_Basic" for the label of the section and "CommIfInfo" for the
   label of the COMM_IF_INFO.

#. In the editor for CommIfInfo, enter the values for VendorName, VendorCode,
   Version and ModelName. Also set these values:

   ========================== ================= =========================
   LABEL                      DATA              Note
   ========================== ================= =========================
   CommIfTypeId               Ethernet
   ReadVersionType            MachineVersion
   SupportIpAddressSize       4
   TimeOutValue               10000             Milliseconds
   SupportFlg SearchNode      1
   SupportFlg SetIpAddress    1
   NumOccupiedStations        1                 Replace with your value
   CCIEFBasicProtocolVersion  2
   EthernetCommFunction       0x00000002
   ========================== ================= =========================

   The remaining SupportFlg values should be 0.

#. Add a COMM_IF_INPUT part to the CommIf_Basic, and use the label "CommIfInput".
   It should have 2 elements. Enter these values:

   ======= =============================== ========== ==============================
   LABEL   NAME                            DATATYPE   REF
   ======= =============================== ========== ==============================
   RX0     Button                          BOOL       BlockSection.BlockOutput.RX0
   RWr0    Button press counter            UINT16     BlockSection.BlockOutput.RWr0
   ======= =============================== ========== ==============================

   The "access" field should be ``RF`` (which is auto refresh) and the "assign"
   field should have same value as the label.

#. Similarly create a COMM_IF_OUTPUT with the label "CommIfOutput" and two elements:
   Use these values:

   ======= =============================== ========== ==============================
   LABEL   NAME                            DATATYPE   REF
   ======= =============================== ========== ==============================
   RY10    LED                             BOOL       BlockSection.BlockInput.RY10
   RWw0    Arbitrary integer from the PLC  UINT16     BlockSection.BlockInput.RWw0
   ======= =============================== ========== ==============================

   The "access" field should be ``RF`` and the "assign" field should have
   same value as the label.


Custom error codes
``````````````````
#. In the BLOCK section add an ENUM part with label "EnumStatusCode" and two elements.
   Use these values:

   ======== ================= ======== ===========================================
   LABEL    NAME              CODE     COMMENT
   ======== ================= ======== ===========================================
   enum1    Wire break        0x0301   Wire break detected. Check all wires.
   enum2    Slave overtemp    0x0302   Slave temperature is too high.
   ======== ================= ======== ===========================================

#. Also in the BLOCK section, add a COMMAND_ARGUMENT with label "StatusArgument"
   and one element. Use these values:

   =========== ============== ======== ==================== ========
   LABEL       NAME           DATATYPE RANGE                ACCESS
   =========== ============== ======== ==================== ========
   StatusCode  Status Code    WORD     ENUM EnumStatusCode  R
   =========== ============== ======== ==================== ========

#. Continue to add a BLOCK_COMMAND to the BLOCK section. Use the label
   "BlockCommand" and one element. Use these values:

   ================= ================================ =================
   LABEL             NAME                             ARGUMENT
   ================= ================================ =================
   GetStatusCommand  Status Code Acquisition Command  StatusArgument
   ================= ================================ =================

#. In the COMM_IF section, add a COMM_IF_COMMAND with label "CCLinkCommand" with
   one element. Use these values:

   ==================== ================================ ===========================================
   LABEL                NAME                             REF
   ==================== ================================ ===========================================
   P_GetStatusCommand   Status Code Acquisition Command  BlockSection.BlockCommand.GetStatusCommand
   ==================== ================================ ===========================================


Saving and exporting the document from the CSP+ tool
----------------------------------------------------
Use the menu :menuselection:`File --> Save` to save your project. This results in a ``.cspproj``
file that is used when you would like to continue your work in the CSP+ tool.

Run an error check by using the menu :menuselection:`File --> Check`.

To export an actual CSP+ file, use the menu :menuselection:`File --> Export`.
The contents needs to be error-free before exporting is possible.
A file name is automatically generated, and it will have the format
``VENDORCODE_MODELNAME_VERSION_LANGUAGE.cspp``. Note that the file name will
contain a space character if there is a space character in the model name.

Several CSP+ files (for example different languages) together with graphics
files can be grouped together in a zip file.
The engineering tool can read this zip file directly. Sometime this archive
is called a "CSP+" (as opposed to the individual "CSP+ files").
Use the CSP+ tool to generate an archive file, by using the menu
:menuselection:`Tool --> "Generate Archive File"`.


Loading a document into the CSP+ tool
-------------------------------------
If a file fails to load, you might need to change the target profile of the tool.
When no project is loaded, it is possible to change the target profile by
using the menu :menuselection:`Tool>"System settings"`.


XML format for CSP+ files
-------------------------
The CSP+ files are in XML format, and often compressed to a single line
(newlines removed).

An XML editor is recommended for working with CSP+ files.
For example the XML editor "QXmlEdit" for Linux is available from the Ubuntu
Software store.

On Windows, the "XML Tools" plugin is available for the Notepad++ editor.

To indent an XML file, use the menu
:menuselection:`Plugins --> "XML Tools" --> "Pretty print(indent attributes)"`.
We prefer to indent with spaces instead of tabs, and this can be done by using the menu
:menuselection:`Settings --> Preferences --> Language --> "Replace by space"`.

Levels:

* Section

   * Part

      * Element (Note that this contains several levels of XML tags)

Each entry in the XML file has information on the field type etc. This
example describes the vendor name, and is called one "Element" in CSP+
nomenclature:

.. code-block:: xml

   <p:deviceInfoMember label="VendorName">
      <p:label2>
         <p:item>Vendor_name</p:item>
      </p:label2>
      <p:category>
         <p:item>COMMON</p:item>
      </p:category>
      <p:name>
         <p:item>Vendor name</p:item>
      </p:name>
      <p:datatype>
         <p:item>STRING_U(64)</p:item>
      </p:datatype>
      <p:data>
         <p:item>RT-Labs AB</p:item>
      </p:data>
   </p:deviceInfoMember>

The XML element hierarchy is here given by the typical ``label`` attributes::

   profile
      FileSection
         FileInfo
            CreateDate
            CreateTime
            ModDate
            ModTime
            Language
            FileVersion
            CCLinkFamilyProfileVersion    2.1 or later
      DeviceSection
         DeviceInfo
            VendorName
            VendorCodedata
            DeviceModel
            ProductID
            DeviceTypeID
            DeviceTypeDetail
            Version
            VersionDisplayFlg
            VersionPolicyType
            DisplayVersionValue
            DeviceConfigurationID
            VersionComment
            ReferenceURL
            URLInfo
            Outline                       Human readable description
            Feature
            SpecList                      List of human readable strings
            PowerSupplyVoltage
            ConsumptionCurrent            in mA
            IconFileName
            GraphicsFileName
            Height                        String including unit
            Width
            Depth
            Weight                        String including unit
            Price                         String including currency
      BlockSection
         BlockInfo
            VendorName
            VendorCode
            Version
         BlockInput
            RY00
            RY01
         BlockOutput
            RX00
            RX01
            RWr00
            RWr01
         BlockCommand
            GetStatusCommand
         EnumStatusCode
            enum1
            enum2
         StatusArgument
            StatusCode
      CommIf_Basic (or CommifSection)
         CommIfInfo
            VendorName
            VendorCode
            CommIFTypeID                     Ethernet for CCIEFB
            Version
            ReadVersionType                  MachineVersion for CCIEFB
            SupportIpAddressSize             4
            TimeOutValue
            SupportFlg_SearchNode            1 for CCIEFB
            SupportFlg_SetIPAddress
            SupportFlg_CompareDeviceInfo
            SupportFlg_GetParam
            SupportFlg_SetParam
            SupportFlg_StartSetParam
            SupportFlg_EndSetParam
            SupportFlg_CancelSetParam
            SupportFlg_ReadStatus
            SupportFlg_GetCommSetting
            SupportFlg_ReadStatus2
            DevModel
            NumOccupiedStations               1 to 4
            CCIEFBasicProtocolVersion         typically 2
            EthernetCommFunction              CCIEFB: 0x00000002 (bit 1 = true)
         CCLinkCommand
            P_GetStatusCommand
         CommifInput
            RX00
            RX01
            RWr00
            RWr01
         CommifOutput
            RY00
            RY01
            RWw00
            RWw01
         CommIfParam
            p_OutputsHoldClear
            p_ModuleIdentification
         Struct

There might be several ``CommIF``. Each supported type of CC-Link
communication protocol has its own COMM_IF section in the CSP+ file.

The hierarchy of XML elements:

.. code-block:: xml

   <p:profile>
      <p:file label="FileSection">
         <p:comment>
            <p:item>
         <p:fileInfo label="FileInfo">
            <p:fileInfoMember label="LABEL FOR THE MEMBER">
               <p:label2>
                  <p:item>
               <p:category>
                  <p:item>
               <p:name>
                  <p:item>
               <p:datatype>
                  <p:item>
               <p:data>
                  <p:item>
      <p:device label="DeviceSection">
         <p:deviceInfo label="DeviceInfo">
            <p:deviceInfoMember label="LABEL FOR THE MEMBER">
               <p:label2>
                  <p:item>
               <p:category>
                  <p:item>
               <p:name>
                  <p:item>
               <p:datatype>
                  <p:item>
               <p:data>
                  <p:item>
      <p:block label="BlockSection">
         <p:blockInfo label="BlockInfo">
            <p:blockInfoMember label="LABEL FOR THE MEMBER">
               <p:label2>
                  <p:item>
               <p:category>
                  <p:item>
               <p:name>
                  <p:item>
               <p:datatype>
                  <p:item>
               <p:data>
                  <p:item>
         <p:blockParameter label="BlockParameter">
            <p:blockParameterMember>
               <p:category>
                  <p:item>
               <p:name>
                  <p:item>
               <p:datatype>
                  <p:item>
               <p:default>
                  <p:item>
               <p:range>
                  <p:enumRefItem>
               <p:access>
                  <p:item>
         <p:enum label="EnumStatusCode">
            <p:enumMember>
               <p:name>
                  <p:item>
               <p:code>
                  <p:item>
         <p:blockInput label="BlockInput">
            <p:blockInputMember>
               <p:category>
                  <p:item>
               <p:name>
                  <p:item>
               <p:datatype>
                  <p:item>
         <p:blockOutput label="BlockOutput">
            <p:blockOutputMember>
               <p:category>
                  <p:item>
               <p:name>
                  <p:item>
               <p:datatype>
                  <p:item>
      <p:commIf label=CommIfSection">
         <p:commIfInfo label="CommIfInfo">
            <p:commIfInfoMember label="LABEL FOR THE MEMBER">
               <p:label2>
                  <p:item>
               <p:category>
                  <p:item>
               <p:name>
                  <p:item>
               <p:datatype>
                  <p:item>
               <p:data>
                  <p:item>
         <p:commIfInput label="CommIfInput">
            <p:commIfInputMember>
               <p:category>
                  <p:item>
               <p:name>
                  <p:item>
               <p:datatype>
                  <p:item>
               <p:access>
                  <p:item>
               <p:assign>
                  <p:item>
               <p:ref>                 optional
                  <p:item>
         <p:commIfOutput label="CommIfOutput">
            <p:commIfOutputMember>
               <p:category>            optional
                  <p:item>
               <p:name>
                  <p:item>
               <p:datatype>
                  <p:item>
               <p:access>
                  <p:item>
               <p:assign>
                  <p:item>
               <p:ref>                 optional
                  <p:item>
         <p:commIfParameter label="CommIfParameter">
            <p:commIfParameterMember>
               <p:category>
                  <p:item>
               <p:name>
                  <p:item>
               <p:datatype>
                  <p:item>             optional
                  <p:structRefItem>    optional
               <p:default>
                  <p:item>
               <p:access>
                  <p:item>
               <p:assign>
                  <p:item>
               <p:ref>
                  <p:item>
               <p:comment>
                  <p:item>
         <p:struct>
            <p:structMember>
               <p:label2>              optional
                  <p:item>
               <p:category>            optional
                  <p:item>
               <p:name>
                  <p:item>
               <p:datatype>
                  <p:item>
               <p:offset>
                  <p:item>
               <p:comment>             optional
                  <p:item>
         <p:enum>
            <p:enumMember>
               <p:label2>
                  <p:item>
               <p:name>
                  <p:item>
               <p:code>
                  <p:item>

# calenhad
Calenhad is GIS and terrain creation for imaginary worlds.

This allows a chain of libnoise modules to be created and edited and used to generate world maps. For a technical summary and acknowledgements, please see about.txt.

To use: you drag noise modules from the palette onto the desktop and connect the output of one to an input of anpther to create connections between them. Double click on the noise module icon on the desktop to bring up a window allowing editing of parameters. On the parameter tab, enter values as numbers or as mathematical expressions; if you want a variable, create it by opening the variables dialog on the Edit menu. Click on the Preview tab to see an overview of the planet generated up to and including that module in the pipeline, and double click on the preview itself to bring up the Marble interactive globe view. Save and load pipelines from the File menu; the XML format it produces should be human-editable. Right-click on the globe view for a context menu supporting some preferences.
  
This has not been tested rigorously at this point and probably has plenty of opportunities to segfault at no notice. It is very much a work in progress and there is still a lot to do. The rendering of maps also runs like a pig and depends on Marble. (see about.txt). Any use made of it is at own risk. 

Compile using cmake in the usual way but be warned that there is a dependency via Marble on QtWebKit. At some point this dependency will be removed.

This work, such as it is, is (c) by Martin Bucknall (chateauferret@yahoo.co.uk) and is made available as is and without warranty of any kind. Comments are welcome, spam is not.

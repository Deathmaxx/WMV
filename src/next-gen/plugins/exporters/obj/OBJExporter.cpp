/*----------------------------------------------------------------------*\
| This file is part of WoW Model Viewer                                  |
|                                                                        |
| WoW Model Viewer is free software: you can redistribute it and/or      |
| modify it under the terms of the GNU General Public License as         |
| published by the Free Software Foundation, either version 3 of the     |
| License, or (at your option) any later version.                        |
|                                                                        |
| WoW Model Viewer is distributed in the hope that it will be useful,    |
| but WITHOUT ANY WARRANTY; without even the implied warranty of         |
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          |
| GNU General Public License for more details.                           |
|                                                                        |
| You should have received a copy of the GNU General Public License      |
| along with WoW Model Viewer.                                           |
| If not, see <http://www.gnu.org/licenses/>.                            |
\*----------------------------------------------------------------------*/

/*
 * OBJExporter.cpp
 *
 *  Created on: 17 feb. 2015
 *   Copyright: 2015 , WoW Model Viewer (http://wowmodelviewer.net)
 */

#define _OBJEXPORTER_CPP_
#include "OBJExporter.h"
#undef _OBJEXPORTER_CPP_

// Includes / class Declarations
//--------------------------------------------------------------------
// STL

// Qt
#include <QFileInfo>
#include <QImage>

// Externals

// Other libraries
#include "CxImage/ximage.h"

#include "Bone.h"
#include "WoWModel.h"

#include "core/GlobalSettings.h"
#include "logger/Logger.h"

#include "metaclasses/Iterator.h"

// Current library


// Namespaces used
//--------------------------------------------------------------------

// Beginning of implementation
//--------------------------------------------------------------------


// Constructors
//--------------------------------------------------------------------


// Destructor
//--------------------------------------------------------------------


// Public methods
//--------------------------------------------------------------------
// Change a Vec3D so it now faces forwards
void MakeModelFaceForwards(Vec3D &vect)
{
  Vec3D Temp;

  Temp.x = 0-vect.z;
  Temp.y = vect.y;
  Temp.z = vect.x;

  vect = Temp;
}

std::string OBJExporter::menuLabel() const
{
  return "OBJ...";
}

std::string OBJExporter::fileSaveTitle() const
{
  return "Save OBJ file";
}

std::string OBJExporter::fileSaveFilter() const
{
  return "OBJ files (*.obj)|*.obj";
}


bool OBJExporter::exportModel(WoWModel * model, std::string target) const
{
  if(!model)
    return false;

  // prepare obj file
  QString targetFile = QString::fromStdString(target);

  QFile file(targetFile);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    LOG_ERROR << "Unable to open" << targetFile;
    return false;
  }

  LOG_INFO << "Exporting" << model->modelname.mb_str() << "in" << targetFile;

  // prepare mtl file
  QString matFilename = QFileInfo(target.c_str()).completeBaseName();
  matFilename += ".mtl";
  matFilename = QFileInfo(target.c_str()).absolutePath () + "/" + matFilename;

  LOG_INFO << "Exporting" << model->modelname.mb_str() << "materials in" << matFilename;

  QFile matFile(matFilename);
  if (!matFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    LOG_ERROR << "Unable to open" << matFilename;
    return false;
  }

  QTextStream obj(&file);
  QTextStream mtl(&matFile);

  obj << "# Wavefront OBJ exported by " << QString::fromStdString(GLOBALSETTINGS.appName()) << " " << QString::fromStdString(GLOBALSETTINGS.appVersion()) << "\n";
  obj << "\n";
  obj << "mtllib " <<  QFileInfo(matFile).fileName() << "\n";
  obj << "\n";


  mtl << "#" << "\n";
  mtl << "# mtl file for " << QFileInfo(targetFile).fileName() << " obj file" << "\n";
  mtl << "#" << "\n";
  mtl << "\n";

  int counter=1;

  // export main model
  if(!exportModelVertices(model, obj, counter))
  {
    LOG_ERROR << "Error during obj export for model" << model->modelname.mb_str();
    return false;
  }

  if(!exportModelMaterials(model, mtl, matFilename))
  {
    LOG_ERROR << "Error during materials export for model" << model->modelname.mb_str();
    return false;
  }

  // export equipped items
  Iterator<WoWItem> itemIt(model);
  for(itemIt.begin(); !itemIt.ended() ; itemIt++)
  {
    std::map<POSITION_SLOTS, WoWModel *> itemModels = (*itemIt)->itemModels;
    if(!itemModels.empty())
    {
      obj << "# " << "\n";
      obj << "# " << QString::fromStdString((*itemIt)->name()) << "\n";
      obj << "# " << "\n";
      for(std::map<POSITION_SLOTS, WoWModel *>::iterator it = itemModels.begin() ;
          it != itemModels.end();
          ++it)
      {
        WoWModel * itemModel = it->second;
        LOG_INFO << "Exporting attached item" << itemModel->modelname.mb_str();

        // find matrix
        int l = model->attLookup[it->first];
        std::cout << "l = " << l << std::endl;
        Matrix m;
        if (l>-1)
          m = model->bones[model->atts[l].bone].mat;

        if(!exportModelVertices(itemModel, obj, counter, m))
        {
          LOG_ERROR << "Error during obj export for model" << itemModel->modelname.mb_str();
          return false;
        }

        if(!exportModelMaterials(itemModel, mtl, matFilename))
        {
          LOG_ERROR << "Error during materials export for model" << itemModel->modelname.mb_str();
          return false;
        }
      }
    }
  }

  file.close();
  matFile.close();

  return true;
}

// Protected methods
//--------------------------------------------------------------------

// Private methods
//--------------------------------------------------------------------
bool OBJExporter::exportModelVertices(WoWModel * model, QTextStream & file, int & counter, Matrix mat) const
{
  //@TODO : do better than that
  QString meshes[NUM_GEOSETS] =
     {"Hairstyles", "Facial1", "Facial2", "Facial3", "Braces",
      "Boots", "", "Ears", "Wristbands",  "Kneepads",
      "Pants", "Pants2", "Tarbard", "Trousers", "Tarbard2",
      "Cape", "Feet", "Eyeglows", "Belt", "Tail" };

  file << "o " << model->modelname.mb_str() << "\n";

  bool vertMsg = false;
  // output all the vertice data
  int vertics = 0;
  for (size_t i=0; i<model->passes.size(); i++)
  {
    ModelRenderPass &p = model->passes[i];

    if (p.init(model))
    {
      //file << "# Chunk Indice Count: " << p.indexCount << endl;

      for (size_t k=0, b=p.indexStart; k<p.indexCount; k++,b++)
      {
        uint16 a = model->indices[b];
        Vec3D vert;
        if ((model->animated == true) && (model->vertices))
        {
          if (vertMsg == false)
          {
            LOG_INFO << "Using Verticies";
            vertMsg = true;
          }
          vert = mat * model->vertices[a];
        }
        else
        {
          if (vertMsg == false)
          {
            LOG_INFO << "Using Original Verticies";
            vertMsg = true;
          }
          vert = mat * model->origVertices[a].pos;
        }
        MakeModelFaceForwards(vert);
        vert *= 1.0;
        file << QString(wxString::Format(wxT("v %.06f %.06f %.06f"), vert.x, vert.y, vert.z).mb_str()) << "\n";

        vertics ++;
      }
    }
  }

  file << "# " << vertics << " vertices" << "\n" << "\n";
  file << "\n";
  // output all the texture coordinate data
  int textures = 0;
  for (size_t i=0; i<model->passes.size(); i++)
  {
    ModelRenderPass &p = model->passes[i];
    // we don't want to render completely transparent parts
    if (p.init(model))
    {
      for (size_t k=0, b=p.indexStart; k<p.indexCount; k++,b++)
      {
        uint16 a = model->indices[b];
        Vec2D tc =  model->origVertices[a].texcoords;
        file << QString(wxString::Format(wxT("vt %.06f %.06f"), tc.x, 1-tc.y)) << "\n";
        //f << "vt " << m->origVertices[a].texcoords.x << " " << (1 - m->origVertices[a].texcoords.y) << endl;
        textures ++;
      }
    }
  }

  // output all the vertice normals data
  int normals = 0;
  for (size_t i=0; i<model->passes.size(); i++)
  {
    ModelRenderPass &p = model->passes[i];
    if (p.init(model))
    {
      for (size_t k=0, b=p.indexStart; k<p.indexCount; k++,b++)
      {
        uint16 a = model->indices[b];
        Vec3D n = model->origVertices[a].normal;
        file << QString(wxString::Format(wxT("vn %.06f %.06f %.06f"), n.x, n.y, n.z)) << "\n";
        //f << "vn " << m->origVertices[a].normal.x << " " << m->origVertices[a].normal.y << " " << m->origVertices[a].normal.z << endl;
        normals ++;
      }
    }
  }

  file << "\n";
  uint32 pointnum = 0;
  // Polygon Data
  int triangles_total = 0;
  for (size_t i=0; i<model->passes.size(); i++)
  {
    ModelRenderPass &p = model->passes[i];

    if (p.init(model))
    {
      // Build Vert2Point DB
      size_t *Vert2Point = new size_t[p.vertexEnd];
      for (size_t v=p.vertexStart; v<p.vertexEnd; v++, pointnum++)
        Vert2Point[v] = pointnum;

      int g = p.geoset;

      QString matName = QString(wxString::Format(wxT("Geoset_%03i"), g));

      if (p.unlit == true)
        matName = matName + "_Lum";

      if (!p.cull)
        matName = matName + "_Dbl";

      // Part Names
      int mesh = model->geosets[g].id / 100;
      QString partName = QString(wxString::Format(wxT("Geoset_%03i"), g));

      if (model->modelType == MT_CHAR && mesh < 19 && meshes[mesh] != wxEmptyString)
      {
        QString msh = meshes[mesh];
        msh.replace(" ", "_");

        partName += QString("-%1").arg(msh);
      }

      file << "g " << partName << "\n";
      file << "usemtl " << matName << "\n";
      file << "s 1" << "\n";
      int triangles = 0;
      for (size_t k=0; k<p.indexCount; k+=3)
      {
        file << "f ";
        file << QString("%1/%1/%1 ").arg(counter);
        counter ++;
        file << QString("%1/%1/%1 ").arg(counter);
        counter ++;
        file << QString("%1/%1/%1\n").arg(counter);
        counter ++;
        triangles ++;
      }
      file << "# " << triangles << " triangles in group" << "\n" << "\n";
      triangles_total += triangles;
    }
  }
  file << "# " << triangles_total << " triangles total" << "\n" << "\n";
  return true;
}

bool OBJExporter::exportModelMaterials(WoWModel * model, QTextStream & file, QString mtlFile) const
{
  std::map<std::string, std::string> texToExport;

  for (size_t i=0; i<model->passes.size(); i++)
  {
    ModelRenderPass &p = model->passes[i];

    if (p.init(model))
    {
      std::string tex = model->TextureList[p.tex].mb_str();
      QString texfile = QFileInfo(tex.c_str()).completeBaseName();
      tex = QFileInfo(mtlFile).completeBaseName().toStdString() + "_" + texfile.toStdString() + ".png";

      float amb = 0.25f;
      Vec4D diff = p.ocol;

      QString material = QString(wxString::Format(wxT("Geoset_%03i"), p.geoset));

      if (p.unlit == true)
      {
        // Add Lum, just in case there's a non-luminous surface with the same name.
        material = material + "_Lum";
        amb = 1.0f;
        diff = Vec4D(0,0,0,0);
      }

      // If Doublesided
      if (!p.cull)
      {
        material = material + "_Dbl";
      }

      file << "newmtl " << material << "\n";
      file << "illum 2" << "\n";
      file << QString(wxString::Format(wxT("Kd %.06f %.06f %.06f"), diff.x, diff.y, diff.z)) << "\n";
      file << QString(wxString::Format(wxT("Ka %.06f %.06f %.06f"), amb, amb, amb)) << "\n";
      file << QString(wxString::Format(wxT("Ks %.06f %.06f %.06f"), p.ecol.x, p.ecol.y, p.ecol.z)) << "\n";
      file << "Ke 0.000000 0.000000 0.000000" << "\n";
      file << QString(wxString::Format(wxT("Ns %0.6f"), 0.0f)) << "\n";

      file << "map_Kd " << QString::fromStdString(tex) << "\n";
      tex = QFileInfo(mtlFile).absolutePath().toStdString() + "\\" + tex;
      texToExport[tex] = model->TextureList[p.tex].mb_str();
    }
  }

  LOG_INFO << "nb textures to export :" << texToExport.size();

  for(std::map<std::string, std::string>::iterator it = texToExport.begin();
      it != texToExport.end();
      ++it)
  {
    if(it->second.find("Body") != std::string::npos)
    {
      exportGLTexture(model->replaceTextures[TEXTURE_BODY], it->first);
    }
    else
    {
      GLuint texID = texturemanager.add(it->second);
      exportGLTexture(texID, it->first);
    }
  }

  return true;
}

void OBJExporter::exportGLTexture(GLuint id, std::string filename) const
{
  LOG_INFO << "Exporting GL texture with id " << id << "in" << filename.c_str();
  glBindTexture(GL_TEXTURE_2D, id);
  unsigned char *pixels = NULL;

  GLint width, height;
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

  pixels = new unsigned char[width * height * 4];

  glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);

  // unfortunatelly, QImage cannot handle tga for writing, use CxImage for now
  if(filename.find(".tga") != std::string::npos)
  {
    CxImage *newImage = new CxImage(0);
    newImage->CreateFromArray(pixels, width, height, 32, (width*4), true);
    newImage->Save(filename.c_str(), CXIMAGE_FORMAT_TGA);
  }
  else
  {
    QImage texture(pixels, width, height,QImage::Format_ARGB32);
    texture.save(filename.c_str());
  }

  /*
  // tga files, starcraft II needs 17th bytes as 8
  if (fn.Last() == 'a') {
    wxFFile f;
    f.Open(fn, wxT("r+b"));
    if (f.IsOpened()) {
      f.Seek(17, wxFromStart);
      char c=8;
      f.Write(&c, sizeof(char));
      f.Close();
    }
  }
  */
}
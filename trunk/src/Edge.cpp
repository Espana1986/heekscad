// Edge.cpp
#include "stdafx.h"
#include "Edge.h"
#include "Solid.h"
#include "../interface/Tool.h"
#include <BRepMesh.hxx>
#include <TColgp_Array1OfDir.hxx>
#include <BRepTools.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopExp.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>

wxIcon* CEdge::m_icon = NULL;

CEdge::CEdge(const TopoDS_Edge &edge):m_topods_edge(edge){
}

CEdge::~CEdge(){
}

void CEdge::glCommands(bool select, bool marked, bool no_color){
	glColor3ub(0, 0, 0);

	if(m_owner && m_owner->m_owner && m_owner->m_owner->GetType() == SolidType)
	{
		// triangulate a face on the edge first
		TopTools_IndexedDataMapOfShapeListOfShape lface;
		TopExp::MapShapesAndAncestors(((CShape*)(m_owner->m_owner))->Shape(),TopAbs_EDGE,TopAbs_FACE,lface);
		const TopTools_ListOfShape& lfac = lface.FindFromKey(m_topods_edge);
		Standard_Integer nelem= lfac.Extent();
		if(nelem == 2){
			TopTools_ListIteratorOfListOfShape It;
			It.Initialize(lfac);
			TopoDS_Face Face1 = TopoDS::Face(It.Value());
			TopLoc_Location fL;
			Handle_Poly_Triangulation facing = BRep_Tool::Triangulation(Face1,fL);

			if(!facing.IsNull())
			{
				// Get polygon
				Handle_Poly_PolygonOnTriangulation polygon = BRep_Tool::PolygonOnTriangulation(m_topods_edge, facing, fL);
				gp_Trsf tr = fL;
				double m[16];
				extract_transposed(tr, m);
				glPushMatrix();
				glMultMatrixd(m);

				if (!polygon.IsNull())
				{
					glBegin(GL_LINE_STRIP);
					const TColStd_Array1OfInteger& Nodes = polygon->Nodes();
					const TColgp_Array1OfPnt& FNodes = facing->Nodes();
					int nnn = polygon->NbNodes();
					for (int nn = 1; nn <= nnn; nn++)
					{
						gp_Pnt v = FNodes(Nodes(nn));
						glVertex3d(v.X(), v.Y(), v.Z());
					}
					glEnd();
				}

				glPopMatrix();
			}
		}
	}
	else
	{
		TopLoc_Location L;
		Handle(Poly_Polygon3D) Polyg = BRep_Tool::Polygon3D(m_topods_edge, L);
		if (!Polyg.IsNull()) {
			const TColgp_Array1OfPnt& Points = Polyg->Nodes();
			Standard_Integer po;
			glBegin(GL_LINE_STRIP);
			for (po = Points.Lower(); po <= Points.Upper(); po++) {
				gp_Pnt p = (Points.Value(po)).Transformed(L);
				glVertex3d(p.X(), p.Y(), p.Z());
			}
			glEnd();
		}
	}
}

void CEdge::GetBox(CBox &box){
}

wxIcon* CEdge::GetIcon(){
	if(m_icon == NULL)
	{
		wxString exe_folder = wxGetApp().GetExeFolder();
		m_icon = new wxIcon(exe_folder + _T("/icons/edge.png"), wxBITMAP_TYPE_PNG);
	}
	return m_icon;
}

class BlendTool:public Tool
{
	CEdge* m_edge;

public:
	BlendTool(CEdge* edge):m_edge(edge){}

	const wxChar* GetTitle(){return _T("Blend");}
	void Run(){
		double rad = 2.0;
		wxGetApp().m_config->Read(_T("EdgeBlendRadius"), &rad);
		if(wxGetApp().InputDouble(_T("Enter Blend Radius"), _T("Radius"), rad))
		{
			m_edge->Blend(rad);
			wxGetApp().m_config->Write(_T("EdgeBlendRadius"), rad);
		}
	}
};

void CEdge::GetTools(std::list<Tool*>* t_list, const wxPoint* p){
	if(m_owner && m_owner->m_owner && m_owner->m_owner->GetType() == SolidType)
		t_list->push_back(new BlendTool(this));
}

void CEdge::Blend(double radius){
	if(m_owner && m_owner->m_owner && CShape::IsTypeAShape(m_owner->m_owner->GetType())){
		BRepFilletAPI_MakeFillet fillet(((CShape*)(m_owner->m_owner))->Shape());
		fillet.Add(radius, m_topods_edge);
		TopoDS_Shape new_shape = fillet.Shape();
		wxGetApp().StartHistory(_T("Blending Edge"));
		wxGetApp().AddUndoably(new CSolid(*((TopoDS_Solid*)(&new_shape)), _T("Edge Blended Solid")), NULL, NULL);
		wxGetApp().DeleteUndoably(m_owner->m_owner);
		wxGetApp().EndHistory();
	}
}

void CEdge::WriteXML(TiXmlElement *root)
{
	CShape::m_solids_found = true;
}

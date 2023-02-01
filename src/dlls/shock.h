#ifndef SHOCKBEAM_H
#define SHOCKBEAM_H

//=========================================================
// Shockrifle projectile
//=========================================================
class CMShock : public CMBaseAnimating
{
public:
	void Spawn(void);
	void Precache(void);
	
	static edict_t *Shoot(entvars_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity);
	void Touch(edict_t *pOther);
	void EXPORT FlyThink();

	void CreateEffects();
	void ClearEffects();
	void UpdateOnRemove();

	CMBeam *m_pBeam;
	CMBeam *m_pNoise;
	CMSprite *m_pSprite;
};
#endif

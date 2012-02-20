echo "Deleating all previous output iamges"
rm def_o.jpg mri_o_a.jpg mri_o_p.jpg
echo "Deforamtion using Moving Least Square Affine transformation"
./deformation -i def_i.jpg -o def_o.jpg -a 2
echo "Registration using Affine transformation"
./registration -i mri_i.jpg -f mri_f.jpg -o mri_o_a.jpg -m affine
echo "Registration using Projective transformation"
./registration -i mri_i.jpg -f mri_f.jpg -o mri_o_p.jpg -m projective
echo "Error between Final image and Affine Registered ouput image"
./registration -i mri_f.jpg -o mri_o_a.jpg -e
echo "Error between Final image and Projetive Registered ouput image"
./registration -i mri_f.jpg -o mri_o_p.jpg -e
